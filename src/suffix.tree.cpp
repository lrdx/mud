#include "suffix.tree.hpp"

#include <stack>
#include <iostream>
#include <algorithm>

#include <cassert>

constexpr std::size_t SuffixTree::Node::TO_THE_END;
constexpr std::size_t SuffixTree::Node::ALPHABET_SIZE;

SuffixTree::Node::Node(const std::size_t from, const std::size_t to /*= TO_THE_END*/) :
	start(from),
	end(to),
	children{ nullptr },
	link(nullptr)
{
}

SuffixTree::Node::~Node()
{
	for (std::size_t i = 0; i != ALPHABET_SIZE; ++i)
	{
		if (children[i])
		{
			delete children[i];
		}
	}
}

SuffixTree::SuffixTree():
	m_root(nullptr)
{
}

void SuffixTree::destroy()
{
	if (m_root)
	{
		delete m_root;
		m_root = nullptr;
	}
}

std::ostream& operator<<(std::ostream& os, const SuffixTree::Position& position)
{
	return position.dump(os);
}

void SuffixTree::build(const std::string& input)
{
	destroy();
	m_input = input + '$';

	m_root = new Node(0, 0);
	Position position(m_root);
	std::size_t remainder = 0;
	std::size_t step = 0;
	for (std::size_t i = 0; i != m_input.size(); ++i)
	{
		Node* prev_node = nullptr;
		++remainder;

		bool first = true;
		while (0 < remainder)
		{
/*
			class Printer
			{
			public:
				Printer(const SuffixTree& tree, const std::size_t i, const SuffixTree::Position& position, const std::size_t& remainder, std::size_t step):
					tree(tree),
					i(i),
					position(position),
					remainder(remainder),
					step(step)
				{
				}
				~Printer()
				{
					tree.dump(std::cout, i) << "......" << std::endl
						<< position << "/" << remainder << std::endl
						<< step << ". --" << std::endl;
				}

			private:
				const SuffixTree& tree;
				const std::size_t i;
				const SuffixTree::Position& position;
				const std::size_t& remainder;
				const std::size_t step;
			} printer(*this, i, position, remainder, ++step);
*/

			// walk down
			if (0 < position.active_length)
			{
				auto child = position.active_node->children[position.active_edge];
				while (0 < position.active_length
					&& child->start + position.active_length >= child->end)
				{
					const auto new_active_length = position.active_length - child->end + child->start;
					const auto new_edge_index = i - position.active_length + 1;
					const auto new_active_edge = m_input[new_edge_index];
/*
					std::cout << "walking down to the node " << child << "; "
						<< new_active_edge << "(" << new_edge_index << "); "
						<< new_active_length << std::endl;
*/
					position.move_to_node(child, new_active_edge, new_active_length);
					child = position.active_node->children[position.active_edge];
				}
			}

			const auto extension = m_input[i];
			if (0 == position.active_length)
			{
				if (nullptr == position.active_node->children[extension])	// no suffix
				{
					position.active_node->children[extension] = new Node(i);
					position.active_node = m_root;
					--remainder;
				}
				else
				{
					position.move_to_edge(extension);

					break;
				}
			}
			else
			{
				const auto active_edge_start = position.active_node->children[position.active_edge]->start;
				const auto next_edge_char = m_input[active_edge_start + position.active_length];
				if (extension == next_edge_char)
				{
					position.move_along_edge();

					const auto child = position.active_node->children[position.active_edge];
					if (!child->contains(position.active_length))
					{
						position.move_to_node(child);
					}

					break;
				}
				else
				{
					// split edge
					const auto node_to_split = position.active_node->children[position.active_edge];
					const auto split_point = node_to_split->start + position.active_length;
					position.active_node->children[position.active_edge] = new Node(node_to_split->start, split_point);
					const auto new_node = position.active_node->children[position.active_edge];
					new_node->children[next_edge_char] = node_to_split;
					node_to_split->start += position.active_length;
					new_node->children[extension] = new Node(i);
					--remainder;

					if (position.active_node == m_root)	// rule 1
					{
						assert(0 < position.active_length);
						const auto new_active_edge = m_input[i - remainder + 1];
						position.rule_1(new_active_edge);
					}
					else	// rule 3
					{
						position.rule_3(m_root);
					}

					if (prev_node)
					{
						prev_node->link = new_node;	// rule 2
					}
					prev_node = new_node;
				}
			}
		}
	}

	if (0 != remainder)
	{
		std::cout << "remainder is not 0" << std::endl;
	}

/*
	dump(std::cout, m_input.size()) << "......" << std::endl
		<< position << "/" << remainder << std::endl
		<< "final tree --" << std::endl;
*/

	return;
}

std::ostream& SuffixTree::dump(std::ostream& os, const std::size_t end) const
{
	using position_t = std::pair<Node*, char_t>;
	using stack_t = std::stack<position_t>;

	stack_t stack;

	stack.emplace(m_root, '\0');
	while (!stack.empty())
	{
		auto& top = stack.top();
		const auto node = top.first;

		const std::size_t level = stack.size();
		if (m_root != node)
		{
			os << std::string(level, ' ') << node << " - "
				<< m_input.substr(node->start, std::min(node->end, end) - node->start);
			if (node->link)
			{
				os << " (link to " << node->link << ")";
			}
			os << std::endl;
		}
		else
		{
			os << "{root}" << std::endl;
		}

		do
		{
			auto& top = stack.top();
			const auto node = top.first;

			do
			{
				++top.second;
			} while ('\0' != top.second
				&& !node->children[top.second]);

			if ('\0' == top.second)	// end of branch
			{
				stack.pop();
			}
			else
			{
				stack.emplace(node->children[top.second], '\0');
				break;
			}
		} while (!stack.empty());
	}

	return os;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
