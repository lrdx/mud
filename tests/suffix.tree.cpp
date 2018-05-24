#include <suffix.tree.hpp>

#include <gtest/gtest.h>

TEST(SuffixTree, Building)
{
	SuffixTree suffix_tree;

	suffix_tree.build("abcabxabcd");
	suffix_tree.dump(std::cout) << "==" << std::endl;
	suffix_tree.build("cdddcdc");
	suffix_tree.dump(std::cout) << "==" << std::endl;
	suffix_tree.build("aaaaa");
	suffix_tree.dump(std::cout) << "==" << std::endl;
	suffix_tree.build("abbaabba");
	suffix_tree.dump(std::cout) << "==" << std::endl;
	suffix_tree.build("ababbaba");
	suffix_tree.dump(std::cout) << "==" << std::endl;

	return;
}