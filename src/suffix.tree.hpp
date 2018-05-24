#ifndef __SUFFIX_TREE_HPP__
#define __SUFFIX_TREE_HPP__

#include <string>

#include <cstddef>

class SuffixTree
{
public:
	using char_t = unsigned char;

	struct Node
	{
		static constexpr std::size_t TO_THE_END = ~0u;
		static constexpr std::size_t ALPHABET_SIZE = 1 << (8*sizeof(char_t));

		Node(const std::size_t from, const std::size_t to = TO_THE_END);
		~Node();

		bool contains(const std::size_t offset) const { return start + offset < end; }

		std::size_t start;
		std::size_t end;

		Node* children[ALPHABET_SIZE];
		Node* link;
	};

	struct Position
	{
		Position(Node* const active_node) : active_node(active_node), active_edge('\0'), active_length(0) {}

		void move_to_edge(const char_t new_active_edge)
		{
			active_edge = new_active_edge;
			++active_length;
		}

		void move_along_edge()
		{
			++active_length;
		}

		void move_to_node(Node* const new_active_node)
		{
			active_node = new_active_node;
			active_edge = '\0';
			active_length = 0;
		}

		void move_to_node(Node* const new_active_node, const char_t new_active_edge, const std::size_t new_active_length)
		{
			active_node = new_active_node;
			active_edge = new_active_edge;
			active_length = new_active_length;
		}

		void rule_1(const char_t new_active_edge)
		{
			active_edge = new_active_edge;
			--active_length;
		}

		void rule_3(Node* const root)
		{
			active_node = active_node->link
				? active_node->link
				: root;
		}

		std::ostream& dump(std::ostream& os) const
		{
			return os << "(" << active_node << "; " << active_edge << "; " << active_length << ")";
		}

		Node* active_node;
		char_t active_edge;
		std::size_t active_length;
	};

	SuffixTree();

	void destroy();
	void build(const std::string& input);
	std::ostream& dump(std::ostream& os) const { return dump(os, m_input.size()); }

private:
	std::ostream& dump(std::ostream& os, const std::size_t end) const;

	Node* m_root;
	std::string m_input;
};

#endif // __SUFFIX_TREE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
