#include "oclint/AbstractASTMatcherRule.h"
#include "oclint/RuleSet.h"

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace oclint;

class KWToolHelper {
public:
    static bool CheckNestedBlocksForTypeCheck(const Stmt *stmt, VarDecl *TempVar) {
        
        bool hasChecked = false;
        if(auto ifSt = dyn_cast<IfStmt>(stmt)) {
            auto conditionExpr = ifSt->getCond();            
            hasChecked = recursiveCheckForObjCFunctionCall(conditionExpr, TempVar);
        }
        if (const auto *binaryExpr = dyn_cast<BinaryOperator>(stmt)) {            
            hasChecked = recursiveCheckForObjCFunctionCall(binaryExpr->getRHS(), TempVar);
        }
        
        
//        
//        if (ObjCBlockExpr *BlockExpr = dyn_cast<clang::ObjCBlockExpr>(Stmt)) {
//            clang::Stmt *BlockBody = BlockExpr->getBody();
//            RecursiveASTVisitor<NSDictionaryEnumerateCheckConsumer> Visitor;
//            Visitor.TraverseStmt(BlockBody);
//
//            for (clang::Stmt *SubStmt : Visitor.getStmts()) {
//                if (CheckNestedBlocksForTypeCheck(SubStmt, TempVar)) {
//                    return true;
//                }
//            }
//        } else if (clang::CallExpr *Call = llvm::dyn_cast<clang::CallExpr>(Stmt)) {
//            for (clang::Expr *Arg : Call->getArgs()) {
//                if (CheckNestedBlocksForTypeCheck(Arg, TempVar)) {
//                    return true;
//                }
//            }
//        } else if (clang::DeclRefExpr *DeclRef = llvm::dyn_cast<clang::DeclRefExpr>(Stmt)) {
//            if (DeclRef->getDecl() == TempVar) {
//                return true;
//            }
//        }
//
        return hasChecked;
    }
    
    static bool recursiveCheckForObjCFunctionCall(const Expr *conditionExpr, const VarDecl *TempVar) {
        if (const auto *parenExpr = dyn_cast<ParenExpr>(conditionExpr)) {
            return recursiveCheckForObjCFunctionCall(parenExpr->getSubExpr(), TempVar);
        }
        else if (const auto *binaryExpr = dyn_cast<BinaryOperator>(conditionExpr)) {
            return recursiveCheckForObjCFunctionCall(binaryExpr->getLHS(), TempVar) ||
            recursiveCheckForObjCFunctionCall(binaryExpr->getRHS(), TempVar);
        }
        else if (const auto *unaryExpr = dyn_cast<UnaryOperator>(conditionExpr)) {
            return recursiveCheckForObjCFunctionCall(unaryExpr->getSubExpr(), TempVar);
        }
    
        else if(const auto *castExpr = dyn_cast<ImplicitCastExpr>(conditionExpr)) {
            return recursiveCheckForObjCFunctionCall(castExpr->getSubExpr(), TempVar);
        }
        
        else if(const auto objcExpr = dyn_cast<ObjCMessageExpr>(conditionExpr)) {
            bool isKindOfSel = objcExpr->getSelector().getAsString() == "isKindOfClass:";
            auto instanceExpr = objcExpr->getInstanceReceiver();
//            if(const auto * implicitExpr = dyn_cast<ImplicitCastExpr>(instanceExpr)) {
//                instanceExpr = implicitExpr->getSubExpr();
//            }
//            
//            if (const DeclRefExpr *D1 = dyn_cast_or_null<DeclRefExpr>(instanceExpr)) {
//                return isKindOfSel && TempVar == D1->getDecl();
//            }
            
            return isKindOfSel && checkVarDecSame(instanceExpr, TempVar);;
        }
        else if (const auto Call = dyn_cast<CallExpr>(conditionExpr)) {
            
            bool isSafeCast = false;
            if(auto decl = dyn_cast<FunctionDecl>(Call->getCalleeDecl())) {
                isSafeCast = decl->getDeclName().getAsString() == "kw_safe_cast";
            }
            bool isSame = checkVarDecSame(*(Call->arg_begin()), TempVar);
            return  isSafeCast && isSame;
        }
        
        return false;
    }
    
    static bool checkVarDecSame(const Expr *exp, const VarDecl *var) {
//        cout<< "变量名称" << var->getQualifiedNameAsString() << endl;
        
        if(const auto * implicitExpr1 = dyn_cast<ImplicitCastExpr>(exp)) {
            return checkVarDecSame(implicitExpr1->getSubExpr(), var);
        }
        if (const DeclRefExpr *D1 = dyn_cast_or_null<DeclRefExpr>(exp)) {
            return var == D1->getDecl();
        }
        
        return false;        
    }
};

class KWCollectionMatchRuleRule : public AbstractASTMatcherRule
{
public:
    virtual const string name() const override
    {
        return "kuwoCollectionMatchRule";
    }

    virtual int priority() const override
    {
        return 3;
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
        return ""; // TODO: fill in the description of the rule.
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
//        const ObjCPropertyDecl *propertyDecl = result.Nodes.getNodeAs<ObjCPropertyDecl>("objcPropertyDecl");
//        if (propertyDecl && isUserSourceDecl(propertyDecl)) {
//            // 存储 Objective-C 类属性
//            propertyDeclVector.push_back(const_cast<ObjCPropertyDecl *>(propertyDecl));
//            
//            checkProperty(propertyDecl);
//        }
        
//        const BinaryOperator *binaryOperator = Result.Nodes.getNodeAs<BinaryOperator>("binaryOperator");
//        if (binaryOperator && isUserSourceStmt(binaryOperator) && binaryOperator->isAssignmentOp()) {
//            checkSameType(binaryOperator);
//        }
        
//        const ObjCInterfaceDecl *objcInterfaceDecl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("objcInterfaceDecl");
//        if (objcInterfaceDecl) {
//            string filename = CI.getSourceManager().getFilename(objcInterfaceDecl->getSourceRange().getBegin()).str();
//            
//            if (!isUserSourceWithFilename(filename)) {
//                return;
//            }
//            
//            checkClassName(objcInterfaceDecl);
//        }
        
        const ObjCMessageExpr *enumerateExp = result.Nodes.getNodeAs<ObjCMessageExpr>("enumerateObjectExpr");
        if (enumerateExp) {
            
            if(auto stmt = dyn_cast<ImplicitCastExpr>(*enumerateExp->arg_begin())) {
                auto block = dyn_cast<BlockExpr>(stmt->getSubExpr());
                
                // loopVar
                auto blockDecl = dyn_cast<BlockDecl>(block->getBlockDecl());
                if(!blockDecl) {
                    return;
                }
                
                //找到for in循环临时变量
                auto loopVarDec = dyn_cast<ParmVarDecl>(*blockDecl->param_begin());
                
                //变量未使用
                if(!loopVarDec->isUsed()) {
                    return;
                }
                
                auto blockBody = blockDecl->getCompoundBody();
                bool res =  KWToolHelper::CheckNestedBlocksForTypeCheck(blockBody->body_front(), loopVarDec);
                if(!res) {
                    addViolation(enumerateExp, this);
                }
            }
            
        }
        
    }

    virtual void setUpMatcher() override
    {
        //属性声明检测
        addMatcher(objcPropertyDecl().bind("objcPropertyDecl"));
        
        //类声明检测
//       addMatcher(objcInterfaceDecl().bind("objcInterfaceDecl"));
        
        //赋值类型检测
//        addMatcher(binaryOperator(hasOperatorName("=")).bind("binaryOperator"));
        
        //enumerateObjectsUsingBlock检测
        addMatcher(objcMessageExpr(hasSelector("enumerateObjectsUsingBlock:")).bind("enumerateObjectExpr"));
    }

};

static RuleSet rules(new KWCollectionMatchRuleRule());
