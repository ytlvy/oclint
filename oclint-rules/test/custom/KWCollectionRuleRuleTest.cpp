#include "TestRuleOnCode.h"
#include "rules/custom/KWCollectionRuleRule.cpp"

TEST(KWCollectionRuleRuleTest, PropertyTest)
{
    KWCollectionRuleRule rule;
    EXPECT_EQ(3, rule.priority());
    EXPECT_EQ("k w collection rule", rule.name());
    EXPECT_EQ("custom", rule.category());
}

TEST(KWCollectionRuleRuleTest, FirstFailingTest)
{
    EXPECT_FALSE("Start writing a new test");
}
