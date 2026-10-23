#pragma once

#include <iostream>
#include <sys/poll.h>
#include <unistd.h>
#include <algorithm>

#include "server/protocols.h"
#include "server/serverstate.h"
#include "utils/socket.h"
#include "utils/signaldesc.h"
#include "tui/sequences.h"
#include "worker.h"
#include "tui/tui.h"
#include "tui/textbox.h"
#include "workers/threadssharedstate.hpp"



template<class T>
struct TUIWorker : public Worker {
public:
    void operator()() {
        SignalDesc signalDesc;
        signalDesc.init({
            SignalDesc::Signal::WindowChange,
            SignalDesc::Signal::Interrupt
        });

        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        std::array<pollfd, 3> fds{};
        auto& pipeFd = fds[0];
        pipeFd.fd = m_commandPipe.getOutput();
        pipeFd.events = POLLIN;
        auto& cinFd = fds[1];
        cinFd.fd = STDIN_FILENO;
        cinFd.events = POLLIN;
        auto& signalFd = fds[2];
        signalFd.fd = signalDesc.getDescriptor();
        signalFd.events = POLLIN;

        AppTUI appTUI;
        { std::lock_guard lock{ss.serverStateInUse};
          setupTUI(appTUI);
        }

        Rectangle::coord_t scrollPos = 0;

        while (true) {
            std::unique_lock stateLock{ss.serverStateInUse};

            updateTUI(appTUI, scrollPos);

            if (appTUI.tui.m_root.dimensions.height >= 20 && appTUI.tui.m_root.dimensions.width >= 20) {
                appTUI.tui.draw();
            }
            std::cout << Sequences::CURSOR_HIDE << Sequences::FG_BLACK << Sequences::BG_WHITE << std::flush;

            stateLock.unlock();

            int result = ::poll(fds.data(), fds.size(), -1);
            if (result == -1) {
                const std::error_code error(errno, std::generic_category());

                if (error == std::errc::interrupted)
                    continue;

                throw std::system_error(error, "TUIWorker::poll");
            }

            bool restart = false;
            if (signalFd.revents & POLLIN) {
                while (auto info = signalDesc.tryReadSignal()) {
                    switch ((SignalDesc::Signal)info->ssi_signo) {
                    case SignalDesc::Signal::WindowChange:
                        appTUI.tui.resizeToScreen();
                        restart = true;
                        break;
                    case SignalDesc::Signal::Interrupt: return;
                    default: break;
                    }
                }
            }

            if (pipeFd.revents & POLLIN) {
                while (auto commandOpt = tryGetCommand()) {
                    switch (commandOpt.value()) {
                    case Command::FinishThread: return;
                    case Command::ResetWorker: restart = true; break;
                    default: break;
                    }
                }
            }
            if (restart) continue;

            if (!(cinFd.revents & POLLIN)) {
                continue;
            }

            bool enterPressed = false;

            while (true) {
                unsigned char ch;
                int result = read(STDIN_FILENO, &ch, 1);

                if (result == -1) {
                    const std::error_code error(errno, std::generic_category());

                    if (error == std::errc::interrupted) continue;
                    else if (error == std::errc::resource_unavailable_try_again) break;
                }
                else if (result == 0) break;

                if (ch == '\n' || ch == '\r') {
                    enterPressed = true;
                    break;
                } else if (ch == 0x7f || ch == '\b') {
                    if (!appTUI.inputField->text.empty()) {
                        appTUI.inputField->text.pop_back();
                    }
                }
                else if (ch == '\x1b') {
                    if (auto sequence = readEscapeSequence()) {
                        switch (sequence.value()) {
                        case Home: scrollPos = 0; break;
                        case KeyUp: scrollPos += 1; break;
                        case KeyDown: if (scrollPos > 0) scrollPos -= 1; break;
                        }
                    }
                }
                else if (std::isprint(static_cast<unsigned char>(ch))) {
                    appTUI.inputField->text.push_back(static_cast<char>(ch));
                }
            }

            if (!enterPressed) {
                continue;
            }

            stateLock.lock();

            std::string newErrorMessage = "";
            std::string_view inputView = appTUI.inputField->text;

            constexpr std::string_view exitCommand = "exit";
            constexpr std::string_view connectCommand = "connect";
            constexpr std::string_view sendCommand = "send";
            constexpr std::string_view chatCommand = "chat";

            if (inputView.starts_with(exitCommand)) {
                executeExit();

                break;
            }
            else if (inputView.starts_with(connectCommand)) {
                executeConnect(inputView.substr(connectCommand.size()), newErrorMessage);
            }
            else if (inputView.starts_with(sendCommand)) {
                executeSend(inputView.substr(sendCommand.size()), newErrorMessage);
            }
            else if (inputView.starts_with(chatCommand)) {
                executeChat(inputView.substr(chatCommand.size()), newErrorMessage);
            }
            else {
                newErrorMessage = "Unknown command";
            }

            appTUI.inputField->text.clear();
            ss.serverState.lastSytemMessage = newErrorMessage;
        }
    }

private:
    std::string_view removeEmptyPrefix(std::string_view view) {
        auto pos = view.find_first_not_of(" \t");
        if (pos == std::string_view::npos) view = {};
        else view.remove_prefix(pos);

        return view;
    }

    enum EscapeSequence {
        Home,
        KeyUp,
        KeyDown
    };
    std::optional<EscapeSequence> readEscapeSequence() {
        char introducer;
        if (read(STDIN_FILENO, &introducer, 1) != 1) return std::nullopt;

        if (introducer != '[' && introducer != 'O') return std::nullopt;

        char final;
        while (read(STDIN_FILENO, &final, 1) == 1) {
            if (final >= '@' && final <= '~') {
                if (final == 'A') return KeyUp;
                if (final == 'B') return KeyDown;
                if (final == 'H') return Home;
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    struct AppTUI {
        TUI tui;

        TextBox* topBar;
        TextBox* verticalLine;
        TextBox* currentUser;
        TextBox* chatName;
        TextBox* errorLine;
        Widget* listViewMessages;
        Widget* listViewChatters;
        TextBox* square;
        TextBox* inputField;

        AppTUI() = default;
        AppTUI(AppTUI&&) = default;
        AppTUI& operator=(AppTUI&&) = default;

        AppTUI(const AppTUI&) = delete;
        AppTUI& operator=(const AppTUI&) = delete;
    };


    void setupTUI(AppTUI& appTUI) {
        ThreadsSharedState& sharedState = static_cast<T&>(*this).getSharedState();

        appTUI.tui.resizeToScreen();

        appTUI.topBar = TextBox::addTextBox(&appTUI.tui.m_root);
        appTUI.topBar->backColor = TextBox::Color::WHITE;

        appTUI.verticalLine = TextBox::addTextBox(&appTUI.tui.m_root);
        appTUI.verticalLine->backColor = TextBox::Color::WHITE;

        appTUI.currentUser = TextBox::addTextBox(&appTUI.tui.m_root);
        appTUI.currentUser->text = "NetMesh <&> "
                                    + sharedState.serverState.clientName
                                    + " on "
                                    + std::to_string(static_cast<T&>(*this).getRegistrationWorker().initial.listenPort);

        appTUI.currentUser->backColor = TextBox::Color::WHITE;
        appTUI.currentUser->frontColor = TextBox::Color::BLACK;

        appTUI.chatName = TextBox::addTextBox(&appTUI.tui.m_root);

        appTUI.errorLine = TextBox::addTextBox(&appTUI.tui.m_root);
        appTUI.errorLine->frontColor = TextBox::Color::RED;

        appTUI.listViewMessages = Widget::addWidget(&appTUI.tui.m_root);
        appTUI.listViewChatters = Widget::addWidget(&appTUI.tui.m_root);

        appTUI.square = TextBox::addTextBox(&appTUI.tui.m_root);
        appTUI.square->backColor = TextBox::Color::YELLOW;

        appTUI.inputField = TextBox::addTextBox(&appTUI.tui.m_root);
        appTUI.inputField->backColor = TextBox::Color::WHITE;
        appTUI.inputField->frontColor = TextBox::Color::BLACK;
    }

    void updateTUI(AppTUI& appTUI, Rectangle::coord_t messagesYOffset) {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        const auto screenWidth = appTUI.tui.m_root.dimensions.width;
        const auto screenHeight = appTUI.tui.m_root.dimensions.height;

        const auto splitOffset = std::max(screenWidth - 10, 6);

        appTUI.topBar->dimensions = {
            .x = 0, .y = 0,
            .width = static_cast<Rectangle::dimension_t>(splitOffset), .height = 1
        };

        appTUI.verticalLine->dimensions = {
            .x = static_cast<Rectangle::coord_t>(splitOffset - 1), .y = 0,
            .width = 1, .height = screenHeight
        };

        appTUI.currentUser->dimensions = {
            .x = 3, .y = 0,
            .width = static_cast<Rectangle::dimension_t>(splitOffset - 3), .height = 1
        };
        appTUI.currentUser->dimensions.width = appTUI.currentUser->text.size();

        appTUI.chatName->dimensions = {
            .x = 0, .y = 1,
            .width = 20, .height = 1
        };

        appTUI.inputField->dimensions = {
            .x = 0,
            .width = static_cast<Rectangle::dimension_t>(splitOffset - 2),
        };
        appTUI.inputField->calculateHeightToFit();
        appTUI.inputField->dimensions.y = screenHeight - std::max<Rectangle::dimension_t>(appTUI.inputField->dimensions.height, 1);

        appTUI.errorLine->dimensions = {
            .x = 0, .y = static_cast<Rectangle::coord_t>(screenHeight - 1 - appTUI.inputField->dimensions.height),
            .width = static_cast<Rectangle::dimension_t>(splitOffset - 1), .height = 1
        };

        appTUI.listViewMessages->dimensions = {
            .x = 0, .y = 2,
            .width = static_cast<Rectangle::dimension_t>(splitOffset - 2),
            .height = static_cast<Rectangle::dimension_t>(screenHeight - 3 - appTUI.inputField->dimensions.height)
        };

        appTUI.square->dimensions = {
            .x = static_cast<Rectangle::coord_t>(splitOffset - 2), .y = 0,
            .width = 2, .height = 2
        };

        appTUI.listViewChatters->dimensions = {
            .x = static_cast<Rectangle::coord_t>(splitOffset), .y = 0,
            .width = 10, .height = screenHeight
        };

        appTUI.listViewMessages->removeAllChild();
        appTUI.listViewChatters->removeAllChild();

        const auto& chatter = ss.serverState.currentChat;
        if (chatter) {
            const Chat* chat = chatter.value();

            appTUI.chatName->frontColor = TextBox::Color::GREEN;
            appTUI.chatName->text = "Chat with " + chat->userName;

            const std::size_t width = appTUI.listViewMessages->dimensions.width;
            std::int16_t remainingHeight = appTUI.listViewMessages->dimensions.height + messagesYOffset;

            for (auto it = chat->history.rbegin();
                 it != chat->history.rend();
                 ++it) {

                TextBox* message = TextBox::addTextBox(appTUI.listViewMessages);
                message->text = it->text;
                message->dimensions = {
                    .x = 7,
                    .width = static_cast<std::uint16_t>(width - 9)
                };
                message->calculateHeightToFit();

                remainingHeight -= message->dimensions.height;
                message->dimensions.y = remainingHeight;

                TextBox* nickname = TextBox::addTextBox(appTUI.listViewMessages);
                nickname->text = (it->own ? ss.serverState.clientName : chat->userName).substr(0, 5) + ": ";
                nickname->dimensions = {
                    .x = 0, .y = remainingHeight,
                    .width = 7, .height = 1
                };
                nickname->frontColor = it->own ? TextBox::Color::YELLOW : TextBox::Color::DEFAULT;

                if (remainingHeight < 0) {
                    break;
                }
            }
        } else {
            appTUI.chatName->frontColor = TextBox::Color::RED;
            appTUI.chatName->text = "No one here...";
        }

        Rectangle::coord_t yOffset = 0;
        int counter = 0;
        for (const Chat& chat : ss.serverState.chats) {
            TextBox* name = TextBox::addTextBox(appTUI.listViewChatters);
            name->text = std::to_string(counter++) + " " + chat.userName;
            name->dimensions = {
                .x = 0, .y = yOffset,
                .width = 10, .height = 1
            };
            name->frontColor = chat.linkedConnection ? TextBox::Color::DEFAULT : TextBox::Color::YELLOW;

            if (!chat.allRead) {
                TextBox* newMess = TextBox::addTextBox(appTUI.listViewChatters);
                newMess->dimensions = {
                    .x = 9, .y = yOffset,
                    .width = 1, .height = 1
                };
                newMess->backColor = TextBox::Color::GREEN;
            }

            ++yOffset;
        }

        if (!ss.serverState.lastSytemMessage.empty()) {
            appTUI.errorLine->text = "~ " + ss.serverState.lastSytemMessage;
        } else {
            appTUI.errorLine->text = {};
        }
    }

    void executeExit() {
        // Placeholder
    }

    void executeConnect(std::string_view params, std::string& newErrorMessage) {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        params = removeEmptyPrefix(params);
        if (params.empty()) {
            newErrorMessage = "No ip address";
            return;
        }

        auto i = params.find_first_of(" \t");
        std::string_view ipAddress = params.substr(0, i);

        if (i >= params.size()) {
            newErrorMessage = "No port";
            return;
        }

        params = params.substr(i);
        params = removeEmptyPrefix(params);

        Socket::port_t port;
        const auto [ptr, ec] = std::from_chars(
            params.data(),
            params.data() + params.size(),
            port
        );

        if (ec != std::errc{}) {
            newErrorMessage = "Invalid port";
            return;
        }

        try {
            Socket socket;
            socket.init();
            socket.connect(ipAddress, port);
            socket.setTimeout(std::chrono::seconds(5));
            socket.setKeepAlive(std::chrono::seconds(1),
                                std::chrono::seconds(1),
                                1);

            std::cout << "Connected to remote!" << std::endl;

            ConnectionState newState{std::move(socket)};

            newState.encryptor.initialize(false);
            const auto pubK = newState.encryptor.getPublickKey();

            Protocols::RawPayload_t buff = Protocols::UserHandshake::start({.pubKey = pubK, .name = ss.serverState.clientName});
            Protocols::Packet_t packet = Protocols::pack(Protocols::PacketType::UserHandshake_Start, buff, newState.encryptor);

            newState.socket.write(packet);

            ss.serverState.connectionsState.emplace_back(std::move(newState));
            static_cast<T&>(*this).getPackageWorker().sendCommand(Command::ResetWorker);
        } catch (const std::exception& e) {
            newErrorMessage = std::string("Error: ") + e.what();
        }
    }

    void executeSend(std::string_view params, std::string& newErrorMessage) {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        if (!ss.serverState.currentChat) {
            newErrorMessage = "Open a chat first";
        }
        else if (!ss.serverState.currentChat.value()->linkedConnection) {
            newErrorMessage = "User is not connected";
        }
        else {
            Chat* chat = ss.serverState.currentChat.value();

            params = removeEmptyPrefix(params);

            if (params.empty()) {
                newErrorMessage = "Message can't be empty";
            }

            std::string message(params);

            Protocols::RawPayload_t buff = Protocols::TextMessage::sendMessage(message);
            Protocols::Packet_t packet = Protocols::pack(Protocols::PacketType::TextMessage_Message, buff, chat->linkedConnection.value()->encryptor);

            chat->linkedConnection.value()->socket.write(packet);
            chat->history.push_back({.own = true, .text = message});
        }
    }

    void executeChat(std::string_view params, std::string& newErrorMessage) {
        ThreadsSharedState& ss = static_cast<T&>(*this).getSharedState();

        params = removeEmptyPrefix(params);

        size_t index{};
        auto [ptr, ec] = std::from_chars(
            params.data(),
            params.data() + params.size(),
            index
        );

        if (ec != std::errc{}) {
            newErrorMessage = "Invalid number";
        } else {
            if (index >= ss.serverState.chats.size()) {
                newErrorMessage = "Who's that...?";
            } else {
                auto it = ss.serverState.chats.begin();
                std::advance(it, index);
                ss.serverState.currentChat = &(*it);
                ss.serverState.currentChat.value()->allRead = true;
            }
        }
    }
};
