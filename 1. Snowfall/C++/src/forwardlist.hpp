#pragma once

#include <atomic>



/** @brief A lock-free forward list class
*
*   For multi-thread use:
*     Access during Read is always safe.
*     Access during Write must be exclusive.
*/
template <class T>
class ForwardList {
private:
	struct Node {
	public:
		T data;
		std::atomic<Node*> next;
		static_assert(std::atomic<Node*>::is_always_lock_free, "ForwardList::Node* isn't atomic!");

		template<class... Args>
		Node(Args&&... args) :
			data(std::forward<Args>(args)...),
			next(nullptr)
		{ }
	};

public:
	struct forward_iterator {
	private:
		Node* current;

	public:
		forward_iterator(Node* node) : current(node) {}

		operator bool() const noexcept { return current != nullptr; }
		bool operator!=(const forward_iterator& right) const noexcept { return current != right.current; }
		void operator++() noexcept { current = current->next; }
		T& operator*() const noexcept { return current->data; }
	};

private:
	std::atomic<Node*> base;


public:
	ForwardList() noexcept : base(nullptr) {}
	~ForwardList() {
		Node* current = base.load();
		while (current) {
			Node* temp = current->next;
			delete(current);
			current = temp;
		}
	}

	template<class... Args>
	void emplace_front(Args&&... args) {
		Node* temp = new Node(std::forward<Args>(args)...);
		Node* baseRaw = base.load();

		temp->next.store(baseRaw);
		base.store(temp);
	}

	void erase(const T& item) noexcept {
		Node* previous = base;
		Node* current = base;
		while (current != nullptr) {
			if (current->data == item) {
				if (current == base) {
					base.store(current->next);
				}
				else {
					previous->next.store(current->next);
				}

				delete(current);

				break;
			}

			previous = current;
			current = current->next;
		}
	}

	bool empty() const noexcept { return base.load() == nullptr; }

	forward_iterator begin() const noexcept { return forward_iterator(base.load()); }
	forward_iterator end() const noexcept { return forward_iterator(nullptr); }
};
