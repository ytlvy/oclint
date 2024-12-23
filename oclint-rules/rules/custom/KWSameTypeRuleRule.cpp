#include "oclint/AbstractASTMatcherRule.h"
#include "oclint/RuleSet.h"

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace oclint;

class KWSameTypeRuleRule : public AbstractASTMatcherRule
{
public:
    virtual const string name() const override
    {
        return "kuwoSameTypeRule";
    }

    virtual int priority() const override
    {
        return 3;
    }
    
    string Description() {
        return "赋值两侧类型不符";
    }
    
    void checkSameType(const BinaryOperator *binaryOperator) {
        ObjCPropertyRefExpr *leftExpr = dyn_cast_or_null<ObjCPropertyRefExpr>(binaryOperator->getLHS());
        OpaqueValueExpr *rightExpr = dyn_cast_or_null<OpaqueValueExpr>(binaryOperator->getRHS());
        if (!leftExpr || !rightExpr) { 
            return;
        }
        
        //如果左边表达式是 Objective-C 类的属性的话，获取该属性对应的类型 A
        std::string propertyName = leftExpr->getGetterSelector().getAsString();
        string leftType = removePtrString(getPropertyType(propertyName));
        
        //如果右边表达式是 Objective-C 的函数调用
        ImplicitCastExpr *castExpr = dyn_cast_or_null<ImplicitCastExpr>(rightExpr->getSourceExpr());
        if (!castExpr) { 
            return;
        }
        
        ObjCMessageExpr *messageExpr = dyn_cast_or_null<ObjCMessageExpr>(castExpr->getSubExpr());
        if (!messageExpr) { 
            return;
        }
        
        //        ObjCMessageExpr *messageExpr = dyn_cast_or_null<ObjCMessageExpr>(rightExpr->getSourceExpr());
        //        if (!messageExpr) { 
        //            return;
        //        }
        
        //检测是否有指定标记 objc_same_type
        ObjCMethodDecl *methodDecl = messageExpr->getMethodDecl(); //函数定义
        if (!checkIfHasAttribute(methodDecl)) {
            //            return;
        }
        
        for (Stmt *stmt : messageExpr->arguments()) {
            ObjCMessageExpr *callClassExpr = dyn_cast_or_null<ObjCMessageExpr>(stmt);
            if (callClassExpr && callClassExpr->getSelector().getAsString() == "class") {
                
                //右值类型
                string rightType = removePtrString(callClassExpr->getClassReceiver().getAsString());
                if (leftType.find(rightType) == std::string::npos) { //类型不相符
                    AppendToViolationSet(propertyDecl, "类型不一致：左边" + leftType + "右边" + rightType );
                }
            }
        }
    }
    
    bool AppendToViolationSet(ObjCForCollectionStmt *node, string description) {
        addViolation(node, this, description);
    }
    
    virtual const string category() const override
    {
        return "custom";
    }

#ifdef DOCGEN
    virtual const std::string since() const override
    {
        return "24.0";
    }

    virtual const std::string description() const override
    {
        return "赋值两侧类型不符"; // TODO: fill in the description of the rule.
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
        const BinaryOperator *binaryOperator = Result.Nodes.getNodeAs<BinaryOperator>("binaryOperator");
        if (binaryOperator && isUserSourceStmt(binaryOperator) && binaryOperator->isAssignmentOp()) {
            checkSameType(binaryOperator);
        }
    }

    virtual void setUpMatcher() override
    {
        //赋值类型检测
        addMatcher(binaryOperator(hasOperatorName("=")).bind("binaryOperator"));
    }

};

static RuleSet rules(new KWSameTypeRuleRule());
