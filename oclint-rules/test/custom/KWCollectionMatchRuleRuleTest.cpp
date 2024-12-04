#include "TestRuleOnCode.h"
#include "rules/custom/KWCollectionMatchRuleRule.cpp"

TEST(KWCollectionMatchRuleRuleTest, PropertyTest)
{
    KWCollectionMatchRuleRule rule;
    EXPECT_EQ(3, rule.priority());
    EXPECT_EQ("k w collection match rule", rule.name());
    EXPECT_EQ("custom", rule.category());
}

TEST(KWCollectionMatchRuleRuleTest, FirstFailingTest)
{
    EXPECT_FALSE("Start writing a new test");
}
