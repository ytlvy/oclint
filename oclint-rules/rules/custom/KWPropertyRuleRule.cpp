#include "oclint/AbstractASTMatcherRule.h"
#include "oclint/RuleSet.h"

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace oclint;

class KWPropertyRuleRule : public AbstractASTMatcherRule
{
public:
    virtual const string name() const override
    {
        return "kuwoPropertyRule";
    }

    virtual int priority() const override
    {
        return 3;
    }

    virtual const string category() const override
    {
        return "custom";
    }

    string Description()
    {
        return "property修饰错误";
    }
    
    bool AppendToViolationSet(const ObjCPropertyDecl *node, string description) {
        addViolation(node, this, description);
    }
    
    bool shouldUseCopy(const string typeStr) {
        if (typeStr.find("NSArray") != string::npos ||
            typeStr.find("NSString") != string::npos ||
            typeStr.find("NSDictionary") != string::npos ) {
            return true;
        }
        return false;
    }
    
    void checkProperty(const ObjCPropertyDecl *propertyDecl) {
        //不可变对象的setter语义推荐使用copy修饰
        string classTypeStr = propertyDecl->getType().getAsString();
        ObjCPropertyAttribute::Kind kind = propertyDecl->getPropertyAttributes();
        if (shouldUseCopy(classTypeStr) && (kind & ObjCPropertyAttribute::Kind::kind_assign)) {
            AppendToViolationSet(propertyDecl, Description());
        }
    }
    
#ifdef DOCGEN
    virtual const std::string since() const override
    {
        return "24.0";
    }

    virtual const std::string description() const override
    {
        return "property修饰错误"; // TODO: fill in the description of the rule.
    }

    virtual const std::string example() const override
    {
        return R"rst(
.. code-block:: cpp

    void example()
    {
        // TODO: modify the example for this rule.
    }
        )rst";
    }

    /* fill in the file name only when it does not match the rule identifier
    virtual const std::string fileName() const override
    {
        return "";
    }
    */

    /* add each threshold's key, description, and its default value to the map.
    virtual const std::map<std::string, std::string> thresholds() const override
    {
        std::map<std::string, std::string> thresholdMapping;
        return thresholdMapping;
    }
    */

    /* add additional document for the rule, like references, notes, etc.
    virtual const std::string additionalDocument() const override
    {
        return "";
    }
    */

    /* uncomment this method when the rule is enabled to be suppressed.
    virtual bool enableSuppress() const override
    {
        return true;
    }
    */
#endif
    
    virtual void callback(const MatchFinder::MatchResult &result) override
    {
        //property copy check 
        const ObjCPropertyDecl *propertyDecl = result.Nodes.getNodeAs<ObjCPropertyDecl>("objcPropertyDecl");
        if (propertyDecl) {
            checkProperty(propertyDecl);
        }
    }

    virtual void setUpMatcher() override
    {
        //属性声明检测
        addMatcher(objcPropertyDecl().bind("objcPropertyDecl"));
        //类声明检测
//        addMatcher(objcInterfaceDecl().bind("objcInterfaceDecl"));
    }

};

static RuleSet rules(new KWPropertyRuleRule());
