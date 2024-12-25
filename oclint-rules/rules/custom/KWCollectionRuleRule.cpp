#include "oclint/AbstractASTVisitorRule.h"
#include "oclint/RuleSet.h"

using namespace std;
using namespace clang;
using namespace oclint;

class KWToolHelper {
public:
    //检测valueVar
    static bool isValueVarFromStringSeparate(const ValueDecl *ivarDec) {
        bool isSplitByStr = false;
        if(nullptr == ivarDec) {
            return isSplitByStr;
        }
        
        if(const auto *var = dyn_cast<VarDecl>(ivarDec)) {
            auto *expr = var->getInit();
            string selector = "componentsSeparatedByString:";
            isSplitByStr = isObjCMessageExprSelector(expr, selector);
            
            if(!isSplitByStr) {
                selector = "tracksWithMediaType:";
                isSplitByStr = isObjCMessageExprSelector(expr, selector);
            }
            
        }
        
        return isSplitByStr;
    }
    
    //检测表达式是否来自obc方法
    static bool isObjCMessageExprSelector(const Expr *expr, string &selector) {
       
        bool isSplitByStr = false;
        if(nullptr == expr) {
            return isSplitByStr;
        }
        
        if(const auto * implicitExpr = dyn_cast<ImplicitCastExpr>(expr)) {
            auto ivarExpr = implicitExpr->getSubExpr();
            isSplitByStr = isObjCMessageExprSelector(ivarExpr, selector);
        }
        else if(const auto * objcExp = dyn_cast<ObjCMessageExpr>(expr)) {
            std::string strArray[] = {"componentsSeparatedByString:",
                "tracksWithMediaType:",
                "matchesInString:options:range:"};
            auto result = std::find(std::begin(strArray), std::end(strArray), objcExp->getSelector().getAsString());
            isSplitByStr = result!= std::end(strArray);
        }
        else if (const auto * conExp = dyn_cast<ConditionalOperator>(expr)) {
            auto *left = conExp->getLHS();
            auto *right = conExp->getRHS();
            
            isSplitByStr = isObjCMessageExprSelector(left, selector) ||
            isObjCMessageExprSelector(right, selector);
            printf("");
        }
        
        return isSplitByStr;
    }
    
    //检测数组是否来源于字符串切割
    static bool CheckArrayFromStringSeparated(Expr *collectionExpr) {
        bool isSplitByStr = false;
        if(nullptr == collectionExpr) {
            return isSplitByStr;
        }
        if(const auto * implicitExpr = dyn_cast<ImplicitCastExpr>(collectionExpr)) {
            auto ivarExpr = implicitExpr->getSubExpr();
            if(nullptr != ivarExpr) {
                if(const auto * decRef = dyn_cast<DeclRefExpr>(ivarExpr)) {
                    auto ivarDec = decRef->getDecl();
                    isSplitByStr = isValueVarFromStringSeparate(ivarDec);
                }
            }
        }
        
        return isSplitByStr;
    }
    
    static int findStmtVarIdex(const ObjCMessageExpr *stmt, VarDecl *TempVar) {
        int indx = 0;
        bool find = false;
        for (auto param = stmt->arg_begin(),
            paramEnd = stmt->arg_end(); param != paramEnd; param++) {
            auto *parmVarDecl = *param;
            if (checkVarDecSame(parmVarDecl, TempVar)) {
                find = true;
                break;
            }
            indx++;
        }
        
        return find ? indx: -1;
    }
    
    static bool CheckFuncOrParmSafeTyped(const Stmt *stmt, VarDecl *TempVar) {
        bool safeChecked = false;
        if(const auto * msgExpr = dyn_cast<ObjCMessageExpr>(stmt)) {//检测调用函数是否添加了
            // 获取消息的接收者
            const Expr *receiver = msgExpr->getInstanceReceiver();
            
            // 获取消息的选择器
            Selector selector = msgExpr->getSelector();
            printf("%s \n", selector.getAsString().c_str()); 
            
            const ObjCMethodDecl *methodDecl = msgExpr->getMethodDecl(); //函数定义
            int idx = findStmtVarIdex(msgExpr, TempVar);
            
            safeChecked = p_CheckFuncOrParmSafeTyped(methodDecl, idx);
            if(safeChecked) {
                return safeChecked;
            }
            
            if(const ObjCInterfaceDecl *interfaceDecl = msgExpr->getReceiverInterface()) {
                for (ObjCMethodDecl *methodDecl : interfaceDecl->methods()) {                    
                    if (methodDecl->getSelector() == selector) {
                        if(p_CheckFuncOrParmSafeTyped(methodDecl, idx)){
                            safeChecked = true;
                        }
                            break;
                    }
                }
            }
        }
        
        return safeChecked;
    }
    
    static bool p_CheckFuncOrParmSafeTyped(const ObjCMethodDecl *decl, int idx) {
        return  checkObjcDeclSafe(decl) || checkObjcDeclParamSafed(decl, idx);
    }
    
    static bool checkObjcDeclSafe(const ObjCMethodDecl *methodDecl)
    {    
        if (methodDecl->hasAttr<AnnotateAttr>()) {
            const AnnotateAttr* annotateAttr = methodDecl->getAttr<AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation();
            if (annotation.find("kw_safe_func")!= std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    static bool isDeclHaveAttr(const Decl *aDecl, const string &attr_name)
    {    
        if (aDecl->hasAttr<AnnotateAttr>()) {
            const AnnotateAttr* annotateAttr = aDecl->getAttr<AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation();
            if (annotation.find(attr_name)!= std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    static bool checkObjcDeclParamSafed(const ObjCMethodDecl *methodDecl, int idx)
    {        
        bool safeCheck = false;
        
        auto visitNameFuc = [&](const ParmVarDecl *parmVarDecl)->void{
            const AnnotateAttr* annotateAttr = parmVarDecl->getAttr<AnnotateAttr>();
            if(annotateAttr == NULL) {
                return;
            }
            auto annotation = annotateAttr->getAnnotation();
            if (annotation.find("kw_safe_param")!= std::string::npos) {
                safeCheck = true;
            }
        };
        
        if (idx < methodDecl->param_size()) {
            const ParmVarDecl *parmVarDecl = methodDecl->getParamDecl((unsigned)idx);
            if (parmVarDecl->getNameAsString() != "")
            {
                visitNameFuc(parmVarDecl);
            }
        }        
        
        return safeCheck;
    }
    
   static bool VisitFunctionDeclParam(const FunctionDecl *decl, VarDecl *TempVar, const std::function<void(const ParmVarDecl *)> &visitNameFuc)
    {
        vector<string> names;
        for (size_t i = 0; i != decl->getNumParams(); ++i)
        {
            const ParmVarDecl *parmVarDecl = decl->getParamDecl((unsigned)i);
            if (parmVarDecl->getNameAsString() != "" && parmVarDecl == TempVar && visitNameFuc)
            {
                visitNameFuc(parmVarDecl);
            }
        }
    }

    static bool VisitObjCMethodDeclParam(const ObjCMethodDecl *decl, VarDecl *TempVar, const std::function<void(const ParmVarDecl *)> &visitNameFuc)
    {
        for (auto param = decl->param_begin(),
            paramEnd = decl->param_end(); param != paramEnd; param++)
        {
            auto *parmVarDecl = *param;
            if (parmVarDecl->getNameAsString() != "" &&  parmVarDecl == TempVar && visitNameFuc)
            {
                visitNameFuc(parmVarDecl);
            }
        }
    }
    
   
    
    static bool checkIfHasAttribute(const ObjCMethodDecl *methodDecl)
    {
        for (Attr *attr : methodDecl->attrs()) {
            if (!strcmp(attr->getSpelling(), "objc_same_type")) {
                return true;
            }
        }
        return false;
    }
    
    //检测forin临时变量是否检测了类型
    static bool CheckNestedBlocksForTypeCheck(const Stmt *stmt, VarDecl *TempVar) {
        
        bool hasChecked = false;
        if(auto ifSt = dyn_cast<IfStmt>(stmt)) {
            auto conditionExpr = ifSt->getCond();            
            hasChecked = recursiveCheckForObjCFunctionCall(conditionExpr, TempVar);
        }
        else if (const auto *binaryExpr = dyn_cast<BinaryOperator>(stmt)) {            
            hasChecked = recursiveCheckForObjCFunctionCall(binaryExpr->getRHS(), TempVar);
        }
        
        if(!hasChecked) {
            hasChecked = CheckFuncOrParmSafeTyped(stmt, TempVar);
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
                isSafeCast = decl->getDeclName().getAsString() == "kw_safe_cast" ||
                decl->getDeclName().getAsString() == "kw_safe_check";
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

class KWCollectionRuleRule : public AbstractASTVisitorRule<KWCollectionRuleRule>
{
public:
    virtual const string name() const override
    {
        return "kuwoCollectionCheckRule";
    }

    virtual int priority() const override
    {
        return 3;
    }

    virtual const string category() const override
    {
        return "custom";
    }
    
    bool AppendToViolationSet(ObjCForCollectionStmt *node, string description) {
        addViolation(node, this, description);
    }
    string Description() 
    {
        return "for循环未进行localVar类型检测";
    }

#ifdef DOCGEN
    virtual const std::string since() const override
    {
        return "24.0";
    }

    virtual const std::string description() const override
    {
        return "for循环未进行localVar类型检测"; // TODO: fill in the description of the rule.
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

    virtual void setUp() override {}
    virtual void tearDown() override {}

    
    /* Visit Stmt
    bool VisitStmt(Stmt *node)
    {
        return true;
    }
     */

    /* Visit NullStmt
    bool VisitNullStmt(NullStmt *node)
    {
        return true;
    }
     */

    /* Visit CompoundStmt
    bool VisitCompoundStmt(CompoundStmt *node)
    {
        return true;
    }
     */

    /* Visit IfStmt
    bool VisitIfStmt(IfStmt *node)
    {
        return true;
    }
     */

    /* Visit SwitchStmt
    bool VisitSwitchStmt(SwitchStmt *node)
    {
        return true;
    }
     */

    /* Visit WhileStmt
    bool VisitWhileStmt(WhileStmt *node)
    {
        return true;
    }
     */

    /* Visit DoStmt
    bool VisitDoStmt(DoStmt *node)
    {
        return true;
    }
     */

    /* Visit ForStmt
    bool VisitForStmt(ForStmt *node)
    {
        return true;
    }
     */

    /* Visit GotoStmt
    bool VisitGotoStmt(GotoStmt *node)
    {
        return true;
    }
     */

    /* Visit IndirectGotoStmt
    bool VisitIndirectGotoStmt(IndirectGotoStmt *node)
    {
        return true;
    }
     */

    /* Visit ContinueStmt
    bool VisitContinueStmt(ContinueStmt *node)
    {
        return true;
    }
     */

    /* Visit BreakStmt
    bool VisitBreakStmt(BreakStmt *node)
    {
        return true;
    }
     */

    /* Visit ReturnStmt
    bool VisitReturnStmt(ReturnStmt *node)
    {
        return true;
    }
     */

    /* Visit DeclStmt 
    bool VisitDeclStmt (DeclStmt  *node)
    {
        return true;
    }
     */

    /* Visit SwitchCase
    bool VisitSwitchCase(SwitchCase *node)
    {
        return true;
    }
     */

    /* Visit CaseStmt
    bool VisitCaseStmt(CaseStmt *node)
    {
        return true;
    }
     */

    /* Visit DefaultStmt
    bool VisitDefaultStmt(DefaultStmt *node)
    {
        return true;
    }
     */

    /* Visit CapturedStmt
    bool VisitCapturedStmt(CapturedStmt *node)
    {
        return true;
    }
     */

    /* Visit ValueStmt
    bool VisitValueStmt(ValueStmt *node)
    {
        return true;
    }
     */

    /* Visit LabelStmt
    bool VisitLabelStmt(LabelStmt *node)
    {
        return true;
    }
     */

    /* Visit AttributedStmt
    bool VisitAttributedStmt(AttributedStmt *node)
    {
        return true;
    }
     */

    /* Visit AsmStmt
    bool VisitAsmStmt(AsmStmt *node)
    {
        return true;
    }
     */

    /* Visit GCCAsmStmt
    bool VisitGCCAsmStmt(GCCAsmStmt *node)
    {
        return true;
    }
     */

    /* Visit MSAsmStmt
    bool VisitMSAsmStmt(MSAsmStmt *node)
    {
        return true;
    }
     */

    /* Visit ObjCAtTryStmt
    bool VisitObjCAtTryStmt(ObjCAtTryStmt *node)
    {
        return true;
    }
     */

    /* Visit ObjCAtCatchStmt
    bool VisitObjCAtCatchStmt(ObjCAtCatchStmt *node)
    {
        return true;
    }
     */

    /* Visit ObjCAtFinallyStmt
    bool VisitObjCAtFinallyStmt(ObjCAtFinallyStmt *node)
    {
        return true;
    }
     */

    /* Visit ObjCAtThrowStmt
    bool VisitObjCAtThrowStmt(ObjCAtThrowStmt *node)
    {
        return true;
    }
     */

    /* Visit ObjCAtSynchronizedStmt
    bool VisitObjCAtSynchronizedStmt(ObjCAtSynchronizedStmt *node)
    {
        return true;
    }
     */

//    Visit ObjCForCollectionStmt
    bool VisitObjCForCollectionStmt(ObjCForCollectionStmt *forStmt)
    {
        if(!forStmt) {
            return false;
        }
        
        // 获取遍历的集合表达式
        Expr *collectionExpr = forStmt->getCollection();
        //数组是否来自于字符串切割
        
        // 获取集合表达式的类型
        const ObjCObjectPointerType *collectionType = dyn_cast<ObjCObjectPointerType>(collectionExpr->getType());
        if (!collectionType || !collectionType->isObjCObjectPointerType()){
            return false;
        }      
        
        const ObjCInterfaceType *objectType = dyn_cast<ObjCInterfaceType>(collectionType->getObjectType());
        if(!objectType) {
            return false;
        }
        
        const std::string className = objectType->getDecl()->getQualifiedNameAsString();
        if(className != "NSArray" && className != "NSDictionary") {
            return false;
        }
        
        // loopVar
        auto loopVarDecStmt = dyn_cast<DeclStmt>(forStmt->getElement());
        if(!loopVarDecStmt) {
            return false;
        }
        
        //找到for in循环临时变量
        auto loopVarDec = dyn_cast<VarDecl>(loopVarDecStmt->getSingleDecl());
        
        //变量未使用
        if(!loopVarDec->isUsed()) {
            return false;
        }
        
        //变量有跳过标记
        if(KWToolHelper::isDeclHaveAttr(loopVarDec, "kw_safe_bypass")) {
            return false;
        }
        
        //找到循环体内使用临时变量的第一个语句
        const auto *Body = dyn_cast<CompoundStmt>(forStmt->getBody());
        if(!Body) {
            return false;
        }
        
        //检测forin遍历变量是否采用了类型检测
        bool res = KWToolHelper::CheckNestedBlocksForTypeCheck(Body->body_front(), loopVarDec);
        
        //是否数组来自字符串切割
        if(!res && KWToolHelper::CheckArrayFromStringSeparated(collectionExpr)) {
            res = true;
        }
        
        if(!res) {
            AppendToViolationSet(forStmt, Description());
        }
        
        return true;
    }
    

    /* Visit ObjCAutoreleasePoolStmt
    bool VisitObjCAutoreleasePoolStmt(ObjCAutoreleasePoolStmt *node)
    {
        return true;
    }
     */

    /* Visit CXXCatchStmt
    bool VisitCXXCatchStmt(CXXCatchStmt *node)
    {
        return true;
    }
     */

    /* Visit CXXTryStmt
    bool VisitCXXTryStmt(CXXTryStmt *node)
    {
        return true;
    }
     */

    /* Visit CXXForRangeStmt
    bool VisitCXXForRangeStmt(CXXForRangeStmt *node)
    {
        return true;
    }
     */

    /* Visit CoroutineBodyStmt
    bool VisitCoroutineBodyStmt(CoroutineBodyStmt *node)
    {
        return true;
    }
     */

    /* Visit CoreturnStmt
    bool VisitCoreturnStmt(CoreturnStmt *node)
    {
        return true;
    }
     */

    /* Visit Expr
    bool VisitExpr(Expr *node)
    {
        return true;
    }
     */

    /* Visit PredefinedExpr
    bool VisitPredefinedExpr(PredefinedExpr *node)
    {
        return true;
    }
     */

    /* Visit DeclRefExpr
    bool VisitDeclRefExpr(DeclRefExpr *node)
    {
        return true;
    }
     */

    /* Visit IntegerLiteral
    bool VisitIntegerLiteral(IntegerLiteral *node)
    {
        return true;
    }
     */

    /* Visit FixedPointLiteral
    bool VisitFixedPointLiteral(FixedPointLiteral *node)
    {
        return true;
    }
     */

    /* Visit FloatingLiteral
    bool VisitFloatingLiteral(FloatingLiteral *node)
    {
        return true;
    }
     */

    /* Visit ImaginaryLiteral
    bool VisitImaginaryLiteral(ImaginaryLiteral *node)
    {
        return true;
    }
     */

    /* Visit StringLiteral
    bool VisitStringLiteral(StringLiteral *node)
    {
        return true;
    }
     */

    /* Visit CharacterLiteral
    bool VisitCharacterLiteral(CharacterLiteral *node)
    {
        return true;
    }
     */

    /* Visit ParenExpr
    bool VisitParenExpr(ParenExpr *node)
    {
        return true;
    }
     */

    /* Visit UnaryOperator
    bool VisitUnaryOperator(UnaryOperator *node)
    {
        return true;
    }
     */

    /* Visit OffsetOfExpr
    bool VisitOffsetOfExpr(OffsetOfExpr *node)
    {
        return true;
    }
     */

    /* Visit UnaryExprOrTypeTraitExpr
    bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *node)
    {
        return true;
    }
     */

    /* Visit ArraySubscriptExpr
    bool VisitArraySubscriptExpr(ArraySubscriptExpr *node)
    {
        return true;
    }
     */

    /* Visit OMPArraySectionExpr
    bool VisitOMPArraySectionExpr(OMPArraySectionExpr *node)
    {
        return true;
    }
     */

    /* Visit OMPIteratorExpr
    bool VisitOMPIteratorExpr(OMPIteratorExpr *node)
    {
        return true;
    }
     */

    /* Visit CallExpr
    bool VisitCallExpr(CallExpr *node)
    {
        return true;
    }
     */

    /* Visit MemberExpr
    bool VisitMemberExpr(MemberExpr *node)
    {
        return true;
    }
     */

    /* Visit CastExpr
    bool VisitCastExpr(CastExpr *node)
    {
        return true;
    }
     */

    /* Visit BinaryOperator
    bool VisitBinaryOperator(BinaryOperator *node)
    {
        return true;
    }
     */

    /* Visit CompoundAssignOperator
    bool VisitCompoundAssignOperator(CompoundAssignOperator *node)
    {
        return true;
    }
     */

    /* Visit AbstractConditionalOperator
    bool VisitAbstractConditionalOperator(AbstractConditionalOperator *node)
    {
        return true;
    }
     */

    /* Visit ConditionalOperator
    bool VisitConditionalOperator(ConditionalOperator *node)
    {
        return true;
    }
     */

    /* Visit BinaryConditionalOperator
    bool VisitBinaryConditionalOperator(BinaryConditionalOperator *node)
    {
        return true;
    }
     */

    /* Visit ImplicitCastExpr
    bool VisitImplicitCastExpr(ImplicitCastExpr *node)
    {
        return true;
    }
     */

    /* Visit ExplicitCastExpr
    bool VisitExplicitCastExpr(ExplicitCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CStyleCastExpr
    bool VisitCStyleCastExpr(CStyleCastExpr *node)
    {
        return true;
    }
     */

    /* Visit OMPArrayShapingExpr
    bool VisitOMPArrayShapingExpr(OMPArrayShapingExpr *node)
    {
        return true;
    }
     */

    /* Visit CompoundLiteralExpr
    bool VisitCompoundLiteralExpr(CompoundLiteralExpr *node)
    {
        return true;
    }
     */

    /* Visit ExtVectorElementExpr
    bool VisitExtVectorElementExpr(ExtVectorElementExpr *node)
    {
        return true;
    }
     */

    /* Visit InitListExpr
    bool VisitInitListExpr(InitListExpr *node)
    {
        return true;
    }
     */

    /* Visit DesignatedInitExpr
    bool VisitDesignatedInitExpr(DesignatedInitExpr *node)
    {
        return true;
    }
     */

    /* Visit DesignatedInitUpdateExpr
    bool VisitDesignatedInitUpdateExpr(DesignatedInitUpdateExpr *node)
    {
        return true;
    }
     */

    /* Visit ImplicitValueInitExpr
    bool VisitImplicitValueInitExpr(ImplicitValueInitExpr *node)
    {
        return true;
    }
     */

    /* Visit NoInitExpr
    bool VisitNoInitExpr(NoInitExpr *node)
    {
        return true;
    }
     */

    /* Visit ArrayInitLoopExpr
    bool VisitArrayInitLoopExpr(ArrayInitLoopExpr *node)
    {
        return true;
    }
     */

    /* Visit ArrayInitIndexExpr
    bool VisitArrayInitIndexExpr(ArrayInitIndexExpr *node)
    {
        return true;
    }
     */

    /* Visit ParenListExpr
    bool VisitParenListExpr(ParenListExpr *node)
    {
        return true;
    }
     */

    /* Visit VAArgExpr
    bool VisitVAArgExpr(VAArgExpr *node)
    {
        return true;
    }
     */

    /* Visit GenericSelectionExpr
    bool VisitGenericSelectionExpr(GenericSelectionExpr *node)
    {
        return true;
    }
     */

    /* Visit PseudoObjectExpr
    bool VisitPseudoObjectExpr(PseudoObjectExpr *node)
    {
        return true;
    }
     */

    /* Visit SourceLocExpr
    bool VisitSourceLocExpr(SourceLocExpr *node)
    {
        return true;
    }
     */

    /* Visit FullExpr
    bool VisitFullExpr(FullExpr *node)
    {
        return true;
    }
     */

    /* Visit ConstantExpr
    bool VisitConstantExpr(ConstantExpr *node)
    {
        return true;
    }
     */

    /* Visit AtomicExpr
    bool VisitAtomicExpr(AtomicExpr *node)
    {
        return true;
    }
     */

    /* Visit AddrLabelExpr
    bool VisitAddrLabelExpr(AddrLabelExpr *node)
    {
        return true;
    }
     */

    /* Visit StmtExpr
    bool VisitStmtExpr(StmtExpr *node)
    {
        return true;
    }
     */

    /* Visit ChooseExpr
    bool VisitChooseExpr(ChooseExpr *node)
    {
        return true;
    }
     */

    /* Visit GNUNullExpr
    bool VisitGNUNullExpr(GNUNullExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXOperatorCallExpr
    bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXMemberCallExpr
    bool VisitCXXMemberCallExpr(CXXMemberCallExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXRewrittenBinaryOperator
    bool VisitCXXRewrittenBinaryOperator(CXXRewrittenBinaryOperator *node)
    {
        return true;
    }
     */

    /* Visit CXXNamedCastExpr
    bool VisitCXXNamedCastExpr(CXXNamedCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXStaticCastExpr
    bool VisitCXXStaticCastExpr(CXXStaticCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXDynamicCastExpr
    bool VisitCXXDynamicCastExpr(CXXDynamicCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXReinterpretCastExpr
    bool VisitCXXReinterpretCastExpr(CXXReinterpretCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXConstCastExpr
    bool VisitCXXConstCastExpr(CXXConstCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXAddrspaceCastExpr
    bool VisitCXXAddrspaceCastExpr(CXXAddrspaceCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXFunctionalCastExpr
    bool VisitCXXFunctionalCastExpr(CXXFunctionalCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXTypeidExpr
    bool VisitCXXTypeidExpr(CXXTypeidExpr *node)
    {
        return true;
    }
     */

    /* Visit UserDefinedLiteral
    bool VisitUserDefinedLiteral(UserDefinedLiteral *node)
    {
        return true;
    }
     */

    /* Visit CXXBoolLiteralExpr
    bool VisitCXXBoolLiteralExpr(CXXBoolLiteralExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXNullPtrLiteralExpr
    bool VisitCXXNullPtrLiteralExpr(CXXNullPtrLiteralExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXThisExpr
    bool VisitCXXThisExpr(CXXThisExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXThrowExpr
    bool VisitCXXThrowExpr(CXXThrowExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXDefaultArgExpr
    bool VisitCXXDefaultArgExpr(CXXDefaultArgExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXDefaultInitExpr
    bool VisitCXXDefaultInitExpr(CXXDefaultInitExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXScalarValueInitExpr
    bool VisitCXXScalarValueInitExpr(CXXScalarValueInitExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXStdInitializerListExpr
    bool VisitCXXStdInitializerListExpr(CXXStdInitializerListExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXNewExpr
    bool VisitCXXNewExpr(CXXNewExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXDeleteExpr
    bool VisitCXXDeleteExpr(CXXDeleteExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXPseudoDestructorExpr
    bool VisitCXXPseudoDestructorExpr(CXXPseudoDestructorExpr *node)
    {
        return true;
    }
     */

    /* Visit TypeTraitExpr
    bool VisitTypeTraitExpr(TypeTraitExpr *node)
    {
        return true;
    }
     */

    /* Visit ArrayTypeTraitExpr
    bool VisitArrayTypeTraitExpr(ArrayTypeTraitExpr *node)
    {
        return true;
    }
     */

    /* Visit ExpressionTraitExpr
    bool VisitExpressionTraitExpr(ExpressionTraitExpr *node)
    {
        return true;
    }
     */

    /* Visit DependentScopeDeclRefExpr
    bool VisitDependentScopeDeclRefExpr(DependentScopeDeclRefExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXConstructExpr
    bool VisitCXXConstructExpr(CXXConstructExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXInheritedCtorInitExpr
    bool VisitCXXInheritedCtorInitExpr(CXXInheritedCtorInitExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXBindTemporaryExpr
    bool VisitCXXBindTemporaryExpr(CXXBindTemporaryExpr *node)
    {
        return true;
    }
     */

    /* Visit ExprWithCleanups
    bool VisitExprWithCleanups(ExprWithCleanups *node)
    {
        return true;
    }
     */

    /* Visit CXXTemporaryObjectExpr
    bool VisitCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXUnresolvedConstructExpr
    bool VisitCXXUnresolvedConstructExpr(CXXUnresolvedConstructExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXDependentScopeMemberExpr
    bool VisitCXXDependentScopeMemberExpr(CXXDependentScopeMemberExpr *node)
    {
        return true;
    }
     */

    /* Visit OverloadExpr
    bool VisitOverloadExpr(OverloadExpr *node)
    {
        return true;
    }
     */

    /* Visit UnresolvedLookupExpr
    bool VisitUnresolvedLookupExpr(UnresolvedLookupExpr *node)
    {
        return true;
    }
     */

    /* Visit UnresolvedMemberExpr
    bool VisitUnresolvedMemberExpr(UnresolvedMemberExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXNoexceptExpr
    bool VisitCXXNoexceptExpr(CXXNoexceptExpr *node)
    {
        return true;
    }
     */

    /* Visit PackExpansionExpr
    bool VisitPackExpansionExpr(PackExpansionExpr *node)
    {
        return true;
    }
     */

    /* Visit SizeOfPackExpr
    bool VisitSizeOfPackExpr(SizeOfPackExpr *node)
    {
        return true;
    }
     */

    /* Visit SubstNonTypeTemplateParmExpr
    bool VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *node)
    {
        return true;
    }
     */

    /* Visit SubstNonTypeTemplateParmPackExpr
    bool VisitSubstNonTypeTemplateParmPackExpr(SubstNonTypeTemplateParmPackExpr *node)
    {
        return true;
    }
     */

    /* Visit FunctionParmPackExpr
    bool VisitFunctionParmPackExpr(FunctionParmPackExpr *node)
    {
        return true;
    }
     */

    /* Visit MaterializeTemporaryExpr
    bool VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *node)
    {
        return true;
    }
     */

    /* Visit LambdaExpr
    bool VisitLambdaExpr(LambdaExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXFoldExpr
    bool VisitCXXFoldExpr(CXXFoldExpr *node)
    {
        return true;
    }
     */

    /* Visit CoroutineSuspendExpr
    bool VisitCoroutineSuspendExpr(CoroutineSuspendExpr *node)
    {
        return true;
    }
     */

    /* Visit CoawaitExpr
    bool VisitCoawaitExpr(CoawaitExpr *node)
    {
        return true;
    }
     */

    /* Visit DependentCoawaitExpr
    bool VisitDependentCoawaitExpr(DependentCoawaitExpr *node)
    {
        return true;
    }
     */

    /* Visit CoyieldExpr
    bool VisitCoyieldExpr(CoyieldExpr *node)
    {
        return true;
    }
     */

    /* Visit ConceptSpecializationExpr
    bool VisitConceptSpecializationExpr(ConceptSpecializationExpr *node)
    {
        return true;
    }
     */

    /* Visit RequiresExpr
    bool VisitRequiresExpr(RequiresExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCStringLiteral
    bool VisitObjCStringLiteral(ObjCStringLiteral *node)
    {
        return true;
    }
     */

    /* Visit ObjCBoxedExpr
    bool VisitObjCBoxedExpr(ObjCBoxedExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCArrayLiteral
    bool VisitObjCArrayLiteral(ObjCArrayLiteral *node)
    {
        return true;
    }
     */

    /* Visit ObjCDictionaryLiteral
    bool VisitObjCDictionaryLiteral(ObjCDictionaryLiteral *node)
    {
        return true;
    }
     */

    /* Visit ObjCEncodeExpr
    bool VisitObjCEncodeExpr(ObjCEncodeExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCMessageExpr
    bool VisitObjCMessageExpr(ObjCMessageExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCSelectorExpr
    bool VisitObjCSelectorExpr(ObjCSelectorExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCProtocolExpr
    bool VisitObjCProtocolExpr(ObjCProtocolExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCIvarRefExpr
    bool VisitObjCIvarRefExpr(ObjCIvarRefExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCPropertyRefExpr
    bool VisitObjCPropertyRefExpr(ObjCPropertyRefExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCIsaExpr
    bool VisitObjCIsaExpr(ObjCIsaExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCIndirectCopyRestoreExpr
    bool VisitObjCIndirectCopyRestoreExpr(ObjCIndirectCopyRestoreExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCBoolLiteralExpr
    bool VisitObjCBoolLiteralExpr(ObjCBoolLiteralExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCSubscriptRefExpr
    bool VisitObjCSubscriptRefExpr(ObjCSubscriptRefExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCAvailabilityCheckExpr
    bool VisitObjCAvailabilityCheckExpr(ObjCAvailabilityCheckExpr *node)
    {
        return true;
    }
     */

    /* Visit ObjCBridgedCastExpr
    bool VisitObjCBridgedCastExpr(ObjCBridgedCastExpr *node)
    {
        return true;
    }
     */

    /* Visit CUDAKernelCallExpr
    bool VisitCUDAKernelCallExpr(CUDAKernelCallExpr *node)
    {
        return true;
    }
     */

    /* Visit ShuffleVectorExpr
    bool VisitShuffleVectorExpr(ShuffleVectorExpr *node)
    {
        return true;
    }
     */

    /* Visit ConvertVectorExpr
    bool VisitConvertVectorExpr(ConvertVectorExpr *node)
    {
        return true;
    }
     */

    /* Visit BlockExpr
    bool VisitBlockExpr(BlockExpr *node)
    {
        return true;
    }
     */

    /* Visit OpaqueValueExpr
    bool VisitOpaqueValueExpr(OpaqueValueExpr *node)
    {
        return true;
    }
     */

    /* Visit TypoExpr
    bool VisitTypoExpr(TypoExpr *node)
    {
        return true;
    }
     */

    /* Visit RecoveryExpr
    bool VisitRecoveryExpr(RecoveryExpr *node)
    {
        return true;
    }
     */

    /* Visit BuiltinBitCastExpr
    bool VisitBuiltinBitCastExpr(BuiltinBitCastExpr *node)
    {
        return true;
    }
     */

    /* Visit MSPropertyRefExpr
    bool VisitMSPropertyRefExpr(MSPropertyRefExpr *node)
    {
        return true;
    }
     */

    /* Visit MSPropertySubscriptExpr
    bool VisitMSPropertySubscriptExpr(MSPropertySubscriptExpr *node)
    {
        return true;
    }
     */

    /* Visit CXXUuidofExpr
    bool VisitCXXUuidofExpr(CXXUuidofExpr *node)
    {
        return true;
    }
     */

    /* Visit SEHTryStmt
    bool VisitSEHTryStmt(SEHTryStmt *node)
    {
        return true;
    }
     */

    /* Visit SEHExceptStmt
    bool VisitSEHExceptStmt(SEHExceptStmt *node)
    {
        return true;
    }
     */

    /* Visit SEHFinallyStmt
    bool VisitSEHFinallyStmt(SEHFinallyStmt *node)
    {
        return true;
    }
     */

    /* Visit SEHLeaveStmt
    bool VisitSEHLeaveStmt(SEHLeaveStmt *node)
    {
        return true;
    }
     */

    /* Visit MSDependentExistsStmt
    bool VisitMSDependentExistsStmt(MSDependentExistsStmt *node)
    {
        return true;
    }
     */

    /* Visit AsTypeExpr
    bool VisitAsTypeExpr(AsTypeExpr *node)
    {
        return true;
    }
     */

    /* Visit DeclDecl
    bool VisitDeclDecl(DeclDecl *node)
    {
        return true;
    }
     */

    /* Visit TranslationUnitDecl
    bool VisitTranslationUnitDecl(TranslationUnitDecl *node)
    {
        return true;
    }
     */

    /* Visit PragmaCommentDecl
    bool VisitPragmaCommentDecl(PragmaCommentDecl *node)
    {
        return true;
    }
     */

    /* Visit PragmaDetectMismatchDecl
    bool VisitPragmaDetectMismatchDecl(PragmaDetectMismatchDecl *node)
    {
        return true;
    }
     */

    /* Visit ExternCContextDecl
    bool VisitExternCContextDecl(ExternCContextDecl *node)
    {
        return true;
    }
     */

    /* Visit NamedDecl
    bool VisitNamedDecl(NamedDecl *node)
    {
        return true;
    }
     */

    /* Visit NamespaceDecl
    bool VisitNamespaceDecl(NamespaceDecl *node)
    {
        return true;
    }
     */

    /* Visit UsingDirectiveDecl
    bool VisitUsingDirectiveDecl(UsingDirectiveDecl *node)
    {
        return true;
    }
     */

    /* Visit NamespaceAliasDecl
    bool VisitNamespaceAliasDecl(NamespaceAliasDecl *node)
    {
        return true;
    }
     */

    /* Visit LabelDecl
    bool VisitLabelDecl(LabelDecl *node)
    {
        return true;
    }
     */

    /* Visit TypeDecl
    bool VisitTypeDecl(TypeDecl *node)
    {
        return true;
    }
     */

    /* Visit TypedefNameDecl
    bool VisitTypedefNameDecl(TypedefNameDecl *node)
    {
        return true;
    }
     */

    /* Visit TypedefDecl
    bool VisitTypedefDecl(TypedefDecl *node)
    {
        return true;
    }
     */

    /* Visit TypeAliasDecl
    bool VisitTypeAliasDecl(TypeAliasDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCTypeParamDecl
    bool VisitObjCTypeParamDecl(ObjCTypeParamDecl *node)
    {
        return true;
    }
     */

    /* Visit UnresolvedUsingTypenameDecl
    bool VisitUnresolvedUsingTypenameDecl(UnresolvedUsingTypenameDecl *node)
    {
        return true;
    }
     */

    /* Visit TagDecl
    bool VisitTagDecl(TagDecl *node)
    {
        return true;
    }
     */

    /* Visit EnumDecl
    bool VisitEnumDecl(EnumDecl *node)
    {
        return true;
    }
     */

    /* Visit RecordDecl
    bool VisitRecordDecl(RecordDecl *node)
    {
        return true;
    }
     */

    /* Visit CXXRecordDecl
    bool VisitCXXRecordDecl(CXXRecordDecl *node)
    {
        return true;
    }
     */

    /* Visit ClassTemplateSpecializationDecl
    bool VisitClassTemplateSpecializationDecl(ClassTemplateSpecializationDecl *node)
    {
        return true;
    }
     */

    /* Visit TemplateTypeParmDecl
    bool VisitTemplateTypeParmDecl(TemplateTypeParmDecl *node)
    {
        return true;
    }
     */

    /* Visit ValueDecl
    bool VisitValueDecl(ValueDecl *node)
    {
        return true;
    }
     */

    /* Visit EnumConstantDecl
    bool VisitEnumConstantDecl(EnumConstantDecl *node)
    {
        return true;
    }
     */

    /* Visit UnresolvedUsingValueDecl
    bool VisitUnresolvedUsingValueDecl(UnresolvedUsingValueDecl *node)
    {
        return true;
    }
     */

    /* Visit IndirectFieldDecl
    bool VisitIndirectFieldDecl(IndirectFieldDecl *node)
    {
        return true;
    }
     */

    /* Visit BindingDecl
    bool VisitBindingDecl(BindingDecl *node)
    {
        return true;
    }
     */

    /* Visit OMPDeclareReductionDecl
    bool VisitOMPDeclareReductionDecl(OMPDeclareReductionDecl *node)
    {
        return true;
    }
     */

    /* Visit OMPDeclareMapperDecl
    bool VisitOMPDeclareMapperDecl(OMPDeclareMapperDecl *node)
    {
        return true;
    }
     */

    /* Visit MSGuidDecl
    bool VisitMSGuidDecl(MSGuidDecl *node)
    {
        return true;
    }
     */

    /* Visit DeclaratorDecl
    bool VisitDeclaratorDecl(DeclaratorDecl *node)
    {
        return true;
    }
     */

    /* Visit FieldDecl
    bool VisitFieldDecl(FieldDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCIvarDecl
    bool VisitObjCIvarDecl(ObjCIvarDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCAtDefsFieldDecl
    bool VisitObjCAtDefsFieldDecl(ObjCAtDefsFieldDecl *node)
    {
        return true;
    }
     */

    /* Visit MSPropertyDecl
    bool VisitMSPropertyDecl(MSPropertyDecl *node)
    {
        return true;
    }
     */

    /* Visit FunctionDecl
    bool VisitFunctionDecl(FunctionDecl *node)
    {
        return true;
    }
     */

    /* Visit CXXDeductionGuideDecl
    bool VisitCXXDeductionGuideDecl(CXXDeductionGuideDecl *node)
    {
        return true;
    }
     */

    /* Visit CXXMethodDecl
    bool VisitCXXMethodDecl(CXXMethodDecl *node)
    {
        return true;
    }
     */

    /* Visit CXXConstructorDecl
    bool VisitCXXConstructorDecl(CXXConstructorDecl *node)
    {
        return true;
    }
     */

    /* Visit CXXDestructorDecl
    bool VisitCXXDestructorDecl(CXXDestructorDecl *node)
    {
        return true;
    }
     */

    /* Visit CXXConversionDecl
    bool VisitCXXConversionDecl(CXXConversionDecl *node)
    {
        return true;
    }
     */

    /* Visit VarDecl
    bool VisitVarDecl(VarDecl *node)
    {
        return true;
    }
     */

    /* Visit VarTemplateSpecializationDecl
    bool VisitVarTemplateSpecializationDecl(VarTemplateSpecializationDecl *node)
    {
        return true;
    }
     */

    /* Visit ImplicitParamDecl
    bool VisitImplicitParamDecl(ImplicitParamDecl *node)
    {
        return true;
    }
     */

    /* Visit ParmVarDecl
    bool VisitParmVarDecl(ParmVarDecl *node)
    {
        return true;
    }
     */

    /* Visit DecompositionDecl
    bool VisitDecompositionDecl(DecompositionDecl *node)
    {
        return true;
    }
     */

    /* Visit OMPCapturedExprDecl
    bool VisitOMPCapturedExprDecl(OMPCapturedExprDecl *node)
    {
        return true;
    }
     */

    /* Visit NonTypeTemplateParmDecl
    bool VisitNonTypeTemplateParmDecl(NonTypeTemplateParmDecl *node)
    {
        return true;
    }
     */

    /* Visit TemplateDecl
    bool VisitTemplateDecl(TemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit RedeclarableTemplateDecl
    bool VisitRedeclarableTemplateDecl(RedeclarableTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit FunctionTemplateDecl
    bool VisitFunctionTemplateDecl(FunctionTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit ClassTemplateDecl
    bool VisitClassTemplateDecl(ClassTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit VarTemplateDecl
    bool VisitVarTemplateDecl(VarTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit TypeAliasTemplateDecl
    bool VisitTypeAliasTemplateDecl(TypeAliasTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit TemplateTemplateParmDecl
    bool VisitTemplateTemplateParmDecl(TemplateTemplateParmDecl *node)
    {
        return true;
    }
     */

    /* Visit BuiltinTemplateDecl
    bool VisitBuiltinTemplateDecl(BuiltinTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit ConceptDecl
    bool VisitConceptDecl(ConceptDecl *node)
    {
        return true;
    }
     */

    /* Visit UsingDecl
    bool VisitUsingDecl(UsingDecl *node)
    {
        return true;
    }
     */

    /* Visit UsingPackDecl
    bool VisitUsingPackDecl(UsingPackDecl *node)
    {
        return true;
    }
     */

    /* Visit UsingShadowDecl
    bool VisitUsingShadowDecl(UsingShadowDecl *node)
    {
        return true;
    }
     */

    /* Visit ConstructorUsingShadowDecl
    bool VisitConstructorUsingShadowDecl(ConstructorUsingShadowDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCMethodDecl
    bool VisitObjCMethodDecl(ObjCMethodDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCContainerDecl
    bool VisitObjCContainerDecl(ObjCContainerDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCCategoryDecl
    bool VisitObjCCategoryDecl(ObjCCategoryDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCProtocolDecl
    bool VisitObjCProtocolDecl(ObjCProtocolDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCInterfaceDecl
    bool VisitObjCInterfaceDecl(ObjCInterfaceDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCCategoryImplDecl
    bool VisitObjCCategoryImplDecl(ObjCCategoryImplDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCImplementationDecl
    bool VisitObjCImplementationDecl(ObjCImplementationDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCPropertyDecl
    bool VisitObjCPropertyDecl(ObjCPropertyDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCCompatibleAliasDecl
    bool VisitObjCCompatibleAliasDecl(ObjCCompatibleAliasDecl *node)
    {
        return true;
    }
     */

    /* Visit LinkageSpecDecl
    bool VisitLinkageSpecDecl(LinkageSpecDecl *node)
    {
        return true;
    }
     */

    /* Visit ExportDecl
    bool VisitExportDecl(ExportDecl *node)
    {
        return true;
    }
     */

    /* Visit ObjCPropertyImplDecl
    bool VisitObjCPropertyImplDecl(ObjCPropertyImplDecl *node)
    {
        return true;
    }
     */

    /* Visit FileScopeAsmDecl
    bool VisitFileScopeAsmDecl(FileScopeAsmDecl *node)
    {
        return true;
    }
     */

    /* Visit AccessSpecDecl
    bool VisitAccessSpecDecl(AccessSpecDecl *node)
    {
        return true;
    }
     */

    /* Visit FriendDecl
    bool VisitFriendDecl(FriendDecl *node)
    {
        return true;
    }
     */

    /* Visit FriendTemplateDecl
    bool VisitFriendTemplateDecl(FriendTemplateDecl *node)
    {
        return true;
    }
     */

    /* Visit StaticAssertDecl
    bool VisitStaticAssertDecl(StaticAssertDecl *node)
    {
        return true;
    }
     */

    /* Visit BlockDecl
    bool VisitBlockDecl(BlockDecl *node)
    {
        return true;
    }
     */

    /* Visit CapturedDecl
    bool VisitCapturedDecl(CapturedDecl *node)
    {
        return true;
    }
     */

    /* Visit ClassScopeFunctionSpecializationDecl
    bool VisitClassScopeFunctionSpecializationDecl(ClassScopeFunctionSpecializationDecl *node)
    {
        return true;
    }
     */

    /* Visit ImportDecl
    bool VisitImportDecl(ImportDecl *node)
    {
        return true;
    }
     */

    /* Visit OMPThreadPrivateDecl
    bool VisitOMPThreadPrivateDecl(OMPThreadPrivateDecl *node)
    {
        return true;
    }
     */

    /* Visit OMPAllocateDecl
    bool VisitOMPAllocateDecl(OMPAllocateDecl *node)
    {
        return true;
    }
     */

    /* Visit OMPRequiresDecl
    bool VisitOMPRequiresDecl(OMPRequiresDecl *node)
    {
        return true;
    }
     */

    /* Visit EmptyDecl
    bool VisitEmptyDecl(EmptyDecl *node)
    {
        return true;
    }
     */

    /* Visit RequiresExprBodyDecl
    bool VisitRequiresExprBodyDecl(RequiresExprBodyDecl *node)
    {
        return true;
    }
     */

    /* Visit LifetimeExtendedTemporaryDecl
    bool VisitLifetimeExtendedTemporaryDecl(LifetimeExtendedTemporaryDecl *node)
    {
        return true;
    }
     */

};

static RuleSet rules(new KWCollectionRuleRule());
