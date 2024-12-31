#include "oclint/AbstractASTMatcherRule.h"
#include "oclint/RuleSet.h"
#include <iostream>

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace oclint;

class KWSameTypeRuleRule : public AbstractASTMatcherRule
{
public:
    static int kGlobalNum;
    
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
    

    
    
    void doCheckSameType(const BinaryOperator *binaryOperator) {
        kGlobalNum ++;
        
        auto left = binaryOperator->getLHS();
        auto right = binaryOperator->getRHS();
        //            std::string propertyName = leftExpr->getGetterSelector().getAsString();

        string lType = "";
        bool isChecked = false;
        if (auto *leftExpr = dyn_cast_or_null<ObjCPropertyRefExpr>(left)) 
        { 
            lType = rtrim(removePtrString(getPropertyType(leftExpr)));
        }
        else  if (auto *leftExpr = dyn_cast_or_null<DeclRefExpr>(left)) 
        {
            lType = getExprType(leftExpr);
            lType = rtrim(removePtrString(lType));
        }
        else  if (auto *leftExpr = dyn_cast_or_null<Expr>(left)) 
        {
            QualType rQType = leftExpr->getType();
            lType = getQualTypeName(rQType);
        }
        
        
        //右侧
        if (OpaqueValueExpr *rExpr = dyn_cast_or_null<OpaqueValueExpr>(right)) 
        { //如果右边表达式是 Objective-C 的函数调用
            if(lType.size()>0 && rExpr != nullptr) {
                isChecked = p_checkSameType(binaryOperator, rExpr, lType);
            }
        }
        else if (ImplicitCastExpr *rightE = dyn_cast_or_null<ImplicitCastExpr>(right)) {
            auto subExpr = rightE->getSubExpr();
            QualType rQType = subExpr->getType();
            if (auto *rExpr1 = dyn_cast_or_null<Expr>(subExpr)) 
            {
                QualType rQType = rExpr1->getType();
                isChecked = p_checkSameType(binaryOperator, rQType, lType);
            }
            
            if(!isChecked) {
                if(auto rExpr = dyn_cast_or_null<Expr>(right)) {
                    isChecked = p_checkSameType(binaryOperator, rExpr, lType);
                }
            }
        }
        else if (ObjCMessageExpr *rExpr = dyn_cast_or_null<ObjCMessageExpr>(right)) 
        { //如果右边表达式是 Objective-C 的函数调用
            if(lType.size()>0 && rExpr != nullptr) {
                isChecked = p_checkSameType(binaryOperator, rExpr, lType);
            }
        }
        else if (ObjCBoxedExpr *rExpr = dyn_cast_or_null<ObjCBoxedExpr>(right)) 
        { //如果右边表达式是 Objective-C 的函数调用
            if(lType.size()>0 && rExpr != nullptr) {
                isChecked = p_checkSameType(binaryOperator, rExpr, lType);
            }
        }
        else if(auto rExpr = dyn_cast_or_null<CXXBoolLiteralExpr>(right)) {
            isChecked =  lType == "_Bool" || lType == "bool";
        }
        else if(auto rExpr = dyn_cast_or_null<CallExpr>(right)) {
            isChecked = p_checkSameType(binaryOperator, rExpr, lType);
        }
        else if(auto rExpr = dyn_cast_or_null<Expr>(right)) {
            isChecked = p_checkSameType(binaryOperator, rExpr, lType);
        }
        
        
        if(isChecked == false) {
            cout << "当前" << kGlobalNum << "未解析成功 left:" << lType << "\n";
            if(lType.size() < 1) {
                left->dump();
            }
            
            printf("当前%d未解析成功 right:\n", kGlobalNum);
            right->dump();
            
            printf("%d 当前%d未解析成功\n", __LINE__, kGlobalNum);
            assert(false);
        }
        
        printf("\n");
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const Expr *expr, string lType) {
        if (auto *castExpr = dyn_cast_or_null<ImplicitCastExpr>(expr)) {
            return p_checkSameType(binaryOperator, castExpr->getSubExpr(), lType);
        }
        else if(auto *pseudoExpr = dyn_cast_or_null<PseudoObjectExpr>(expr)){
            return p_checkSameType(binaryOperator, pseudoExpr->getResultExpr(), lType);
        }
        else if(auto messageExpr = dyn_cast_or_null<ObjCMessageExpr>(expr)) {
            return p_checkSameType(binaryOperator, messageExpr, lType);
        }
        else if(auto E = dyn_cast_or_null<DeclRefExpr>(expr)) {
            return p_checkSameType(binaryOperator, E, lType);
        }
        else if(auto *callE = dyn_cast_or_null<CallExpr>(expr)) {
            return p_checkSameType(binaryOperator, callE, lType);
        }
        else if(auto E = dyn_cast_or_null<CXXOperatorCallExpr>(expr)) {
            return p_checkSameType(binaryOperator, E, lType);
        }
        else if(auto E = dyn_cast_or_null<CXXMemberCallExpr>(expr)) {
            return p_checkSameType(binaryOperator, E, lType);
        }
        else if(auto E = dyn_cast_or_null<CXXBindTemporaryExpr>(expr)) {
            auto subE = E->getSubExpr();
            return p_checkSameType(binaryOperator, subE, lType);
        }
        
        QualType rQType = expr->getType();
        p_checkSameType(binaryOperator, rQType, lType);
        return true; 
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const CallExpr *expr, string lType) {
        QualType rQType = expr->getCallReturnType(*_carrier->getASTContext());
        p_checkSameType(binaryOperator, rQType, lType);
        return true; 
    }
    
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const CXXMemberCallExpr *expr, string lType) {
        QualType rQType = expr->getCallReturnType(*_carrier->getASTContext());
        p_checkSameType(binaryOperator, rQType, lType);
        return true; 
    }
    
    
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const QualType &rQType, string lType) {
        string opaqueName = getQualTypeName(rQType);
        
        //        const Expr *callee = expr->getCallee();
        //        string rType = "";
        //        if(auto *E = dyn_cast_or_null<ImplicitCastExpr>(callee)) {
        //            auto *subE = E->getSubExpr();
        //            if(auto *declE = dyn_cast_or_null<DeclRefExpr>(subE)) {
        //                rType = declE->getType().getAsString();
        //                replaceAll(rType, opaqueName+" (", "");
        //                replaceAll(rType, ")", "");
        //            }
        //        }
        
        p_checkSameType(binaryOperator, opaqueName, lType);
        return true; 
    }
    
    //隐式对象
    bool p_checkSameType(const BinaryOperator *binaryOperator, const OpaqueValueExpr *rightExpr, string leftType) {
        auto *expr = rightExpr->getSourceExpr();
        return p_checkSameType(binaryOperator, expr, leftType);
    }

    
//    string getDeclRefExprType(const DeclRefExpr *expr) {
//        string tname = expr->getType().getAsString();
//        removeEnumString(tname);
//        return tname;
//    }
    
//    string geObjCBoxedExprType(const ObjCBoxedExpr *expr) {
//        string tname = expr->getType().getAsString();
//        removeEnumString(tname);
//        return tname;
//    }

    string getQualTypeName(const QualType &rQType) {
        string opaqueName = "";
        if(auto ET = dyn_cast_or_null<ElaboratedType>(rQType)) {
            QualType DT = ET->desugar();
            opaqueName = DT.getAsString();
        }
        else {
            opaqueName = rQType.getAsString();
        }
        removeEnumString(opaqueName);
        return opaqueName;
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const CXXOperatorCallExpr *expr, string lType) {
       
        QualType rQType = expr->getCallReturnType(*_carrier->getASTContext());
        string opaqueName = getQualTypeName(rQType);
        
        const Expr *callee = expr->getCallee();
        string rType = "";
        if(auto *E = dyn_cast_or_null<ImplicitCastExpr>(callee)) {
            auto *subE = E->getSubExpr();
            if(auto *declE = dyn_cast_or_null<DeclRefExpr>(subE)) {
                rType = declE->getType().getAsString();
                replaceAll(rType, opaqueName+" (", "");
                replaceAll(rType, ")", "");
            }
        }
        
        p_checkSameType(binaryOperator, rType, lType);
        return true; 
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const ObjCBoxedExpr *expr, string lType) {
        string rType = getExprType(expr);
        p_checkSameType(binaryOperator, rType, lType);
        return true; 
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const DeclRefExpr *expr, string lType) {
        string rType = getExprType(expr); 
        p_checkSameType(binaryOperator, rType, lType);
        return true; 
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const ObjCMessageExpr *messageExpr, string leftType) {
        
        //检测是否有指定标记 objc_same_type
        const ObjCMethodDecl *methodDecl = messageExpr->getMethodDecl(); //函数定义
        string selector = messageExpr->getSelector().getAsString();
        if(selector == "init" || selector == "new" || selector == "initWithFrame:") {//初始化不做处理
            return true;
        }
        
        auto resType = methodDecl->getReturnType();
        if(auto T = dyn_cast_or_null<AttributedType>(resType)){
            auto T1 = T->desugar();
            if(auto type = dyn_cast_or_null<ObjCTypeParamType>(T1)) {
                if(auto ty = dyn_cast_or_null<ObjCObjectPointerType>(type->desugar())) {
                    return p_checkSameType(binaryOperator, ty, leftType);
                }
                else {
                    printf("%d 当前未解析成功 right: %d\n", __LINE__, kGlobalNum);
                    resType->dump();
                }
            }
            else if(auto ty = dyn_cast_or_null<ObjCObjectPointerType>(T1)) {
                return p_checkSameType(binaryOperator, ty, leftType);
            }
            else {
                printf("%d 当前未解析成功 right: %d\n", __LINE__, kGlobalNum);
                T1->dump();
            }
        }
        else if(auto ty = dyn_cast_or_null<ObjCObjectPointerType>(resType)) {
            return p_checkSameType(binaryOperator, ty, leftType);
        }
        else if(const TypedefType *ty = dyn_cast_or_null<TypedefType>(resType)) {
            
            QualType dq = ty->desugar();
            return p_checkSameType(binaryOperator, dq, leftType);
        }
        else if(auto ty = dyn_cast_or_null<BuiltinType>(resType)) {
            return p_checkSameType(binaryOperator, ty, leftType);
        }
        
        printf("%d 当前未解析成功 left:%s | right: %d\n", __LINE__, leftType.c_str(), kGlobalNum);
        resType->dump();
        
        return false;
    }
    
    bool startWith(const std::string& str, const std::string& prefix) {
        if (str.length() >= prefix.length()) {
            std::string sub = str.substr(0, prefix.length());
            return sub == prefix;
        }
        return false;
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const BuiltinType *expr, string lType) {
        
        bool isNumber = expr->isInteger() || expr->isFloatingType();
        if(isNumber) {
            return true;
        }
        
        return false; 
    }

    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const string &rName, const string &lName) {
        string rrName = lrtrim(removePtrString(rName));
        removeEnumString(rrName);
        getNormlizationType(rrName);
        
        string llName = lName;
        getNormlizationType(llName);
        
        if(rrName != llName) {
            
            if(lName=="id") {
                std::cout << __LINE__ << " 类型不一致" << "左边: " + lName + " <=> 右边: " + rName <<" 当前:" << kGlobalNum <<endl;    
            }
            else {
                std::cout << __LINE__ << " 类型不一致" << "左边: " + lName + " <=> 右边: " + rName <<" 当前:" << kGlobalNum <<endl;
                AppendToViolationSet(binaryOperator, "赋值两侧类型不一致 左边:" + lName + " - 右边:" + rName);
            }
            return true;
        }
        else {
            std::cout << __LINE__ << " 类型一致" << lName <<"当前:" << kGlobalNum <<endl;
            return true;
        }
    }
    
    bool p_checkSameType(const BinaryOperator *binaryOperator, const ObjCObjectPointerType *ty, string leftType) {
        string rname =  getObjcObjectType(ty);
        p_checkSameType(binaryOperator, rname, leftType);
        return true;
    } 
    
    bool isObjcTypeId(const ObjCObjectPointerType *objType) {
        bool ret = false;
        auto type = objType->getObjectType();
        if(auto ty = dyn_cast_or_null<ObjCObjectType>(type)) {
            ret = ty->isObjCId();
        }
        
        return ret;
    }
    
    string rtrim(const std::string& str) {
        size_t endPos = str.find_last_not_of(" \t\n\r\f\v");
        if (endPos!= std::string::npos) {
            return str.substr(0, endPos + 1);
        }
        return "";
    }
    
    std::string lrtrim(const std::string& str) {
        // 去除左边空白字符
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) {
            return "";
        }
        // 去除右边空白字符
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }
    
    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
    
    void removeEnumString(string &typeString)
    {
        replaceAll(typeString, "enum ", "");
        replaceAll(typeString, "class ", "");
        replaceAll(typeString, " _Nullable", "");
    }
    
    
    void getNormlizationType(string &typeString)
    {
        replaceAll(typeString, "float", "CGFloat");
        replaceAll(typeString, "double", "CGFloat");
        replaceAll(typeString, "int8_t", "NSInteger");
        replaceAll(typeString, "int", "NSInteger");
        replaceAll(typeString, "int16_t", "NSInteger");
        replaceAll(typeString, "int32_t", "NSInteger");
        replaceAll(typeString, "int64_t", "NSInteger");
        replaceAll(typeString, "const", "");
        replaceAll(typeString, "  ", " ");
        replaceAll(typeString, "NSUInteger", "NSInteger");
        typeString = removePtrString(typeString);
        typeString = lrtrim(typeString);
    }
    
    string removePtrString(const string typeString)
    {
        size_t lastindex = typeString.find_last_of("*");
        return typeString.substr(0, lastindex);
    }
    
    
    string getExprType(const Expr *expr) {
        string tname = expr->getType().getAsString();
        removeEnumString(tname);
        return tname;
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
            doCheckSameType(binaryOperator);
        }
    }

    virtual void setUpMatcher() override
    {
        //赋值类型检测
        addMatcher(binaryOperator(hasOperatorName("=")).bind("binaryOperator"));
    }

};

int KWSameTypeRuleRule::kGlobalNum = 0;
static RuleSet rules(new KWSameTypeRuleRule());
