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
    
    bool checkIfHasAttribute(ObjCMethodDecl *methodDecl)
    {
        for (Attr *attr : methodDecl->attrs()) {
            if (!strcmp(attr->getSpelling(), "objc_same_type")) {
                return true;
            }
        }
        return false;
    }

    string getPropertyType(const ObjCPropertyRefExpr *expr)
    {
        if(!expr->isImplicitProperty()) {
            ObjCPropertyDecl * decl = expr->getExplicitProperty();
            if(decl){
                return decl->getType().getAsString();
            }
        }

        return "";
    }
    
    string removePtrString(const string typeString)
    {
        size_t lastindex = typeString.find_last_of("*");
        return typeString.substr(0, lastindex);
    }
    
    void checkSameType(const BinaryOperator *binaryOperator) {
        ObjCPropertyRefExpr *leftExpr = dyn_cast_or_null<ObjCPropertyRefExpr>(binaryOperator->getLHS());
        OpaqueValueExpr *rightExpr = dyn_cast_or_null<OpaqueValueExpr>(binaryOperator->getRHS());
        if (!leftExpr || !rightExpr) { 
            return;
        }
        
        //如果左边表达式是 Objective-C 类的属性的话，获取该属性对应的类型 A
        std::string propertyName = leftExpr->getGetterSelector().getAsString();
        string leftType = removePtrString(getPropertyType(leftExpr));
        
        auto *sourceExpr = rightExpr->getSourceExpr();
        //如果右边表达式是 Objective-C 的函数调用
        
    
        if (ObjCMessageExpr *messageExpr = dyn_cast_or_null<ObjCMessageExpr>(rightExpr->getSourceExpr())) {
            p_checkSameType(binaryOperator, messageExpr, leftType);
            return;
        }
        
        if (ImplicitCastExpr *castExpr = dyn_cast_or_null<ImplicitCastExpr>(sourceExpr)) {
            ObjCMessageExpr *messageExpr = dyn_cast_or_null<ObjCMessageExpr>(castExpr->getSubExpr());
            if (messageExpr) {
                p_checkSameType(binaryOperator, messageExpr, leftType);
            }
        }
        
        if(auto *pseudoExpr = dyn_cast_or_null<PseudoObjectExpr>(sourceExpr)){
            ObjCMessageExpr *messageExpr = dyn_cast_or_null<ObjCMessageExpr>(pseudoExpr->getResultExpr());
            if (messageExpr) {
                p_checkSameType(binaryOperator, messageExpr, leftType);
            };
            
        }
        printf("");
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const ObjCMessageExpr *messageExpr, string leftType) {
        
        //检测是否有指定标记 objc_same_type
        const ObjCMethodDecl *methodDecl = messageExpr->getMethodDecl(); //函数定义
        
        auto resType = methodDecl->getReturnType();
        if(auto T = dyn_cast_or_null<AttributedType>(resType)){
            if(auto type = dyn_cast_or_null<ObjCTypeParamType>(T->desugar())) {
                if(auto ty = dyn_cast_or_null<ObjCObjectPointerType>(type->desugar())) {
                    string tname =  getObjcObjectType(ty);
                    if(leftType != tname) {
                        AppendToViolationSet(binaryOperator, "类型不一致：左边" + leftType + "右边" + tname);
                    }
                }
            }
        }
    }
    
    bool isObjcTypeId(const ObjCObjectPointerType *objType) {
        bool ret = false;
        auto type = objType->getObjectType();
        if(auto ty = dyn_cast_or_null<ObjCObjectType>(type)) {
            ret = ty->isObjCId();
        }
        
        return ret;
    }
    
    string getObjcObjectType(const ObjCObjectPointerType *objType)
    {
        string ret = "";
        auto type = objType->getObjectType();
        if(const ObjCObjectType *ty = dyn_cast_or_null<ObjCObjectType>(type)) {
            QualType T1 = ty->desugar();
            ret = T1.getAsString();
        }
        return ret;
    }
    
    string findObjcExprParamClassType(const ObjCMessageExpr *messageExpr) {
        string rightType = "";
        for (auto *stmt : messageExpr->arguments()) {
            auto *callClassExpr = dyn_cast_or_null<ObjCMessageExpr>(stmt);
            if (callClassExpr && callClassExpr->getSelector().getAsString() == "class") {
                rightType = removePtrString(callClassExpr->getClassReceiver().getAsString());
                break;
            }
        }
        return rightType;
    }
    
//    if (leftType.find(rightType) == std::string::npos) { //类型不相符
//        
//    }
    
    bool AppendToViolationSet(const BinaryOperator *binaryOperator, const string &description) {
        addViolation(binaryOperator, this, description);
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
        const BinaryOperator *binaryOperator = result.Nodes.getNodeAs<BinaryOperator>("binaryOperator");
        if (binaryOperator && binaryOperator->isAssignmentOp()) {
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
