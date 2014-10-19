
// Compiler implementation of the D programming language
// Copyright (c) 1999-2011 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// http://www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#ifndef DMD_DECLARATION_H
#define DMD_DECLARATION_H

#ifdef __DMC__
#pragma once
#endif /* __DMC__ */

#if IN_LLVM
#include <set>
#include <map>
#include <string>
#if LDC_LLVM_VER >= 305
#include "llvm/IR/DebugInfo.h"
#elif LDC_LLVM_VER >= 302
#include "llvm/DebugInfo.h"
#else
#include "llvm/Analysis/DebugInfo.h"
#endif
#endif

#include "dsymbol.h"
#include "lexer.h"
#include "mtype.h"

class Expression;
class Statement;
class LabelDsymbol;
class Initializer;
class Module;
struct InlineScanState;
class ForeachStatement;
class FuncDeclaration;
class ExpInitializer;
class StructDeclaration;
struct InterState;
struct IRState;
struct CompiledCtfeFunction;
#if IN_LLVM
class AnonDeclaration;
class LabelStatement;
#endif

enum PROT;
enum LINK;
enum TOK;
enum MATCH;
enum PURE;

#define STCundefined    0LL
#define STCstatic       1LL
#define STCextern       2LL
#define STCconst        4LL
#define STCfinal        8LL
#define STCabstract     0x10LL
#define STCparameter    0x20LL
#define STCfield        0x40LL
#define STCoverride     0x80LL
#define STCauto         0x100LL
#define STCsynchronized 0x200LL
#define STCdeprecated   0x400LL
#define STCin           0x800LL         // in parameter
#define STCout          0x1000LL        // out parameter
#define STClazy         0x2000LL        // lazy parameter
#define STCforeach      0x4000LL        // variable for foreach loop
#define STCcomdat       0x8000LL        // should go into COMDAT record
#define STCvariadic     0x10000LL       // variadic function argument
#define STCctorinit     0x20000LL       // can only be set inside constructor
#define STCtemplateparameter  0x40000LL // template parameter
#define STCscope        0x80000LL       // template parameter
#define STCimmutable    0x100000LL
#define STCref          0x200000LL
#define STCinit         0x400000LL      // has explicit initializer
#define STCmanifest     0x800000LL      // manifest constant
#define STCnodtor       0x1000000LL     // don't run destructor
#define STCnothrow      0x2000000LL     // never throws exceptions
#define STCpure         0x4000000LL     // pure function
#define STCtls          0x8000000LL     // thread local
#define STCalias        0x10000000LL    // alias parameter
#define STCshared       0x20000000LL    // accessible from multiple threads
#define STCgshared      0x40000000LL    // accessible from multiple threads
                                        // but not typed as "shared"
#define STCwild         0x80000000LL    // for "wild" type constructor
#define STC_TYPECTOR    (STCconst | STCimmutable | STCshared | STCwild)
#define STC_FUNCATTR    (STCref | STCnothrow | STCpure | STCproperty | STCsafe | STCtrusted | STCsystem)

#define STCproperty      0x100000000LL
#define STCsafe          0x200000000LL
#define STCtrusted       0x400000000LL
#define STCsystem        0x800000000LL
#define STCctfe          0x1000000000LL  // can be used in CTFE, even if it is static
#define STCdisable       0x2000000000LL  // for functions that are not callable
#define STCresult        0x4000000000LL  // for result variables passed to out contracts
#define STCnodefaultctor 0x8000000000LL  // must be set inside constructor
#define STCtemp          0x10000000000LL // temporary variable
#define STCrvalue        0x20000000000LL // force rvalue for variables

const StorageClass STCStorageClass = (STCauto | STCscope | STCstatic | STCextern | STCconst | STCfinal |
    STCabstract | STCsynchronized | STCdeprecated | STCoverride | STClazy | STCalias |
    STCout | STCin |
    STCmanifest | STCimmutable | STCshared | STCwild | STCnothrow | STCpure | STCref | STCtls |
    STCgshared | STCproperty | STCsafe | STCtrusted | STCsystem | STCdisable);

struct Match
{
    int count;                  // number of matches found
    MATCH last;                 // match level of lastf
    FuncDeclaration *lastf;     // last matching function we found
    FuncDeclaration *nextf;     // current matching function
    FuncDeclaration *anyf;      // pick a func, any func, to use for error recovery
};

void functionResolve(Match *m, Dsymbol *fd, Loc loc, Scope *sc, Objects *tiargs, Type *tthis, Expressions *fargs);
int overloadApply(Dsymbol *fstart, void *param, int (*fp)(void *, Dsymbol *));

void ObjectNotFound(Identifier *id);

enum Semantic
{
    SemanticStart,      // semantic has not been run
    SemanticIn,         // semantic() is in progress
    SemanticDone,       // semantic() has been run
    Semantic2Done,      // semantic2() has been run
};

/**************************************************************/

class Declaration : public Dsymbol
{
public:
    Type *type;
    Type *originalType;         // before semantic analysis
    StorageClass storage_class;
    PROT protection;
    LINK linkage;
    int inuse;                  // used to detect cycles
    const char *mangleOverride;      // overridden symbol with pragma(mangle, "...")
    Semantic sem;

    // CALYPSO
    virtual LangPlugin *langPlugin() { return NULL; }

    Declaration(Identifier *id);
    void semantic(Scope *sc);
    const char *kind();
    unsigned size(Loc loc);
    int checkModify(Loc loc, Scope *sc, Type *t, Expression *e1, int flag);

    Dsymbol *search(Loc loc, Identifier *ident, int flags = IgnoreNone);

    void emitComment(Scope *sc);
    void toDocBuffer(OutBuffer *buf, Scope *sc);

    const char *mangle(bool isv = false);
    bool isStatic() { return (storage_class & STCstatic) != 0; }
    virtual bool isDelete();
    virtual bool isDataseg();
    virtual bool isThreadlocal();
    virtual bool isCodeseg();
    bool isCtorinit()     { return (storage_class & STCctorinit) != 0; }
    bool isFinal()        { return (storage_class & STCfinal) != 0; }
    bool isAbstract()     { return (storage_class & STCabstract) != 0; }
    bool isConst()        { return (storage_class & STCconst) != 0; }
    bool isImmutable()    { return (storage_class & STCimmutable) != 0; }
    bool isWild()         { return (storage_class & STCwild) != 0; }
    bool isAuto()         { return (storage_class & STCauto) != 0; }
    bool isScope()        { return (storage_class & STCscope) != 0; }
    bool isSynchronized() { return (storage_class & STCsynchronized) != 0; }
    bool isParameter()    { return (storage_class & STCparameter) != 0; }
    bool isDeprecated()   { return (storage_class & STCdeprecated) != 0; }
    bool isOverride()     { return (storage_class & STCoverride) != 0; }
    bool isResult()       { return (storage_class & STCresult) != 0; }
    bool isField()        { return (storage_class & STCfield) != 0; }

    bool isIn()    { return (storage_class & STCin) != 0; }
    bool isOut()   { return (storage_class & STCout) != 0; }
    bool isRef()   { return (storage_class & STCref) != 0; }

    PROT prot();

    Declaration *isDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

/**************************************************************/

class TupleDeclaration : public Declaration
{
public:
    Objects *objects;
    bool isexp;                 // true: expression tuple

    TypeTuple *tupletype;       // !=NULL if this is a type tuple

    TupleDeclaration(Loc loc, Identifier *ident, Objects *objects);
    Dsymbol *syntaxCopy(Dsymbol *);
    const char *kind();
    Type *getType();
    bool needThis();

    TupleDeclaration *isTupleDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }

#if IN_LLVM
    void semantic3(Scope *sc);
#endif
};

/**************************************************************/

class TypedefDeclaration : public Declaration
{
public:
    Type *basetype;
    Initializer *init;

    TypedefDeclaration(Loc loc, Identifier *ident, Type *basetype, Initializer *init);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void semantic2(Scope *sc);
    const char *mangle(bool isv = false);
    const char *kind();
    Type *getType();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *htype;
    Type *hbasetype;

    void toDocBuffer(OutBuffer *buf, Scope *sc);

#if IN_DMD
    void toObjFile(int multiobj);                       // compile to .obj file
    void toDebug();
    int cvMember(unsigned char *p);
#endif

    TypedefDeclaration *isTypedefDeclaration() { return this; }

#if IN_DMD
    Symbol *sinit;
    Symbol *toInitializer();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

/**************************************************************/

class AliasDeclaration : public Declaration
{
public:
    Dsymbol *aliassym;
    Dsymbol *overnext;          // next in overload list
    Dsymbol *import;            // !=NULL if unresolved internal alias for selective import
    bool inSemantic;

    AliasDeclaration(Loc loc, Identifier *ident, Type *type);
    AliasDeclaration(Loc loc, Identifier *ident, Dsymbol *s);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    bool overloadInsert(Dsymbol *s);
    const char *kind();
    Type *getType();
    Dsymbol *toAlias();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *htype;
    Dsymbol *haliassym;

    void toDocBuffer(OutBuffer *buf, Scope *sc);

    AliasDeclaration *isAliasDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

/**************************************************************/

class VarDeclaration : public Declaration
{
public:
    Initializer *init;
    unsigned offset;
    bool noscope;                // no auto semantics
    FuncDeclarations nestedrefs; // referenced by these lexically nested functions
    bool isargptr;              // if parameter that _argptr points to
    structalign_t alignment;
    bool ctorinit;              // it has been initialized in a ctor
    short onstack;              // 1: it has been allocated on the stack
                                // 2: on stack, run destructor anyway
    int canassign;              // it can be assigned to
    bool overlapped;            // if it is a field and has overlapping
    Dsymbol *aliassym;          // if redone as alias to another symbol
    VarDeclaration *lastVar;    // Linked list of variables for goto-skips-init detection

    // When interpreting, these point to the value (NULL if value not determinable)
    // The index of this variable on the CTFE stack, -1 if not allocated
    int ctfeAdrOnStack;
    // The various functions are used only to detect compiler CTFE bugs
    Expression *getValue();
    bool hasValue();
    void setValueNull();
    void setValueWithoutChecking(Expression *newval);
    void setValue(Expression *newval);

    VarDeclaration *rundtor;    // if !NULL, rundtor is tested at runtime to see
                                // if the destructor should be run. Used to prevent
                                // dtor calls on postblitted vars
    Expression *edtor;          // if !=NULL, does the destruction of the variable

    VarDeclaration(Loc loc, Type *t, Identifier *id, Initializer *init);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void setFieldOffset(AggregateDeclaration *ad, unsigned *poffset, bool isunion);
    void semantic2(Scope *sc);
    const char *kind();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Type *htype;
    Initializer *hinit;
    AggregateDeclaration *isThis();
    bool needThis();
    bool isExport();
    bool isImportedSymbol();
    bool isDataseg();
    bool isThreadlocal();
    bool isCTFE();
    bool hasPointers();
    bool canTakeAddressOf();
    bool needsAutoDtor();
    Expression *callScopeDtor(Scope *sc);
    ExpInitializer *getExpInitializer();
    Expression *getConstInitializer(bool needFullType = true);
    void checkCtorConstInit();
    void checkNestedReference(Scope *sc, Loc loc);
    Dsymbol *toAlias();
#if IN_DMD
    Symbol *toSymbol();
    void toObjFile(int multiobj);                       // compile to .obj file
    int cvMember(unsigned char *p);
#endif
    const char *mangle(bool isv = false);
    // Eliminate need for dynamic_cast
    VarDeclaration *isVarDeclaration() { return (VarDeclaration *)this; }
    void accept(Visitor *v) { v->visit(this); }

#if IN_LLVM
    /// Index into parent aggregate.
    /// Set during type generation.
    unsigned aggrIndex;

    /// Variables that wouldn't have gotten semantic3'ed if we weren't inlining set this flag.
    bool availableExternally;
    /// Override added to set above flag.
    void semantic3(Scope *sc);

    /// This var is used by a naked function.
    bool nakedUse;

    // debug description
    llvm::DIVariable debugVariable;
    llvm::DISubprogram debugFunc;
#endif
};

/**************************************************************/

// This is a shell around a back end symbol

class SymbolDeclaration : public Declaration
{
public:
    StructDeclaration *dsym;

    SymbolDeclaration(Loc loc, StructDeclaration *dsym);

#if IN_DMD
    Symbol *toSymbol();
#endif

    // Eliminate need for dynamic_cast
    SymbolDeclaration *isSymbolDeclaration() { return (SymbolDeclaration *)this; }
    void accept(Visitor *v) { v->visit(this); }
};

class ClassInfoDeclaration : public VarDeclaration
{
public:
    ClassDeclaration *cd;

    ClassInfoDeclaration(ClassDeclaration *cd);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);

    void emitComment(Scope *sc);

#if IN_DMD
    Symbol *toSymbol();
#endif

    ClassInfoDeclaration* isClassInfoDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoDeclaration : public VarDeclaration
{
public:
    Type *tinfo;

    TypeInfoDeclaration(Type *tinfo, int internal);
    static TypeInfoDeclaration *create(Type *tinfo, int internal);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    char *toChars();

    void emitComment(Scope *sc);

#if IN_DMD
    Symbol *toSymbol();
    void toObjFile(int multiobj);                       // compile to .obj file
    virtual void toDt(dt_t **pdt);
#endif
    TypeInfoDeclaration *isTypeInfoDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }

#if IN_LLVM
    virtual void llvmDefine();
#endif
};

class TypeInfoStructDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoStructDeclaration(Type *tinfo);
    static TypeInfoStructDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoClassDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoClassDeclaration(Type *tinfo);
    static TypeInfoClassDeclaration *create(Type *tinfo);

#if IN_DMD
    Symbol *toSymbol();
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    // TypeInfoClassDeclaration instances are different; they describe
    // __ClassZ/__InterfaceZ symbols instead of a TypeInfo_….init one. DMD also
    // generates them for SomeInterface.classinfo access, so we can't just
    // distinguish between them using tinfo and thus need to override codegen().
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoInterfaceDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoInterfaceDeclaration(Type *tinfo);
    static TypeInfoInterfaceDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoTypedefDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoTypedefDeclaration(Type *tinfo);
    static TypeInfoTypedefDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoPointerDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoPointerDeclaration(Type *tinfo);
    static TypeInfoPointerDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoArrayDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoArrayDeclaration(Type *tinfo);
    static TypeInfoArrayDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoStaticArrayDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoStaticArrayDeclaration(Type *tinfo);
    static TypeInfoStaticArrayDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoAssociativeArrayDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoAssociativeArrayDeclaration(Type *tinfo);
    static TypeInfoAssociativeArrayDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoEnumDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoEnumDeclaration(Type *tinfo);
    static TypeInfoEnumDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoFunctionDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoFunctionDeclaration(Type *tinfo);
    static TypeInfoFunctionDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoDelegateDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoDelegateDeclaration(Type *tinfo);
    static TypeInfoDelegateDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoTupleDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoTupleDeclaration(Type *tinfo);
    static TypeInfoTupleDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoConstDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoConstDeclaration(Type *tinfo);
    static TypeInfoConstDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoInvariantDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoInvariantDeclaration(Type *tinfo);
    static TypeInfoInvariantDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoSharedDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoSharedDeclaration(Type *tinfo);
    static TypeInfoSharedDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoWildDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoWildDeclaration(Type *tinfo);
    static TypeInfoWildDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

class TypeInfoVectorDeclaration : public TypeInfoDeclaration
{
public:
    TypeInfoVectorDeclaration(Type *tinfo);
    static TypeInfoVectorDeclaration *create(Type *tinfo);

#if IN_DMD
    void toDt(dt_t **pdt);
#endif

#if IN_LLVM
    void llvmDefine();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

/**************************************************************/

class ThisDeclaration : public VarDeclaration
{
public:
    ThisDeclaration(Loc loc, Type *t);
    Dsymbol *syntaxCopy(Dsymbol *);
    ThisDeclaration *isThisDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

enum ILS
{
    ILSuninitialized,   // not computed yet
    ILSno,              // cannot inline
    ILSyes,             // can inline
};

/**************************************************************/

enum BUILTIN
{
    BUILTINunknown = -1,        // not known if this is a builtin
    BUILTINno,                  // this is not a builtin
    BUILTINyes,                 // this is a builtin
};

Expression *eval_builtin(Loc loc, FuncDeclaration *fd, Expressions *arguments);

typedef Expression *(*builtin_fp)(Loc loc, FuncDeclaration *fd, Expressions *arguments);
void add_builtin(const char *mangle, builtin_fp fp);
void builtin_init();

class FuncDeclaration : public Declaration
{
public:
    Types *fthrows;                     // Array of Type's of exceptions (not used)
    Statement *frequire;
    Statement *fensure;
    Statement *fbody;

    FuncDeclarations foverrides;        // functions this function overrides
    FuncDeclaration *fdrequire;         // function that does the in contract
    FuncDeclaration *fdensure;          // function that does the out contract

#if IN_LLVM
    // Argument lists for the __require/__ensure calls. NULL if not a virtual
    // function with contracts.
    Expressions *fdrequireParams;
    Expressions *fdensureParams;
#endif

    Identifier *outId;                  // identifier for out statement
    VarDeclaration *vresult;            // variable corresponding to outId
    LabelDsymbol *returnLabel;          // where the return goes
    Scope *scout;                       // out contract scope for vresult->semantic

    DsymbolTable *localsymtab;          // used to prevent symbols in different
                                        // scopes from having the same name
    VarDeclaration *vthis;              // 'this' parameter (member and nested)
    VarDeclaration *v_arguments;        // '_arguments' parameter
#ifdef IN_GCC
    VarDeclaration *v_arguments_var;    // '_arguments' variable
    VarDeclaration *v_argptr;           // '_argptr' variable
#endif
    VarDeclaration *v_argsave;          // save area for args passed in registers for variadic functions
    VarDeclarations *parameters;        // Array of VarDeclaration's for parameters
    DsymbolTable *labtab;               // statement label symbol table
    Dsymbol *overnext;                  // next in overload list
    FuncDeclaration *overnext0;         // next in overload list (only used during IFTI)
    Loc endloc;                         // location of closing curly bracket
    int vtblIndex;                      // for member functions, index into vtbl[]
    bool naked;                         // true if naked
    ILS inlineStatusStmt;
    ILS inlineStatusExp;

    CompiledCtfeFunction *ctfeCode;     // Compiled code for interpreter
    int inlineNest;                     // !=0 if nested inline
#if IN_LLVM
    char isArrayOp;                     // 1 if compiler-generated array op, 2 if druntime-provided
#else
    bool isArrayOp;                     // true if array operation
#endif
    FuncDeclaration *dArrayOp;          // D version of array op for ctfe
    bool semantic3Errors;               // true if errors in semantic3
                                        // this function's frame ptr
    ForeachStatement *fes;              // if foreach body, this is the foreach
    bool introducing;                   // true if 'introducing' function
    Type *tintro;                       // if !=NULL, then this is the type
                                        // of the 'introducing' function
                                        // this one is overriding
    bool inferRetType;                  // true if return type is to be inferred
    StorageClass storage_class2;        // storage class for template onemember's

    // Things that should really go into Scope
    int hasReturnExp;                   // 1 if there's a return exp; statement
                                        // 2 if there's a throw statement
                                        // 4 if there's an assert(0)
                                        // 8 if there's inline asm

    // Support for NRVO (named return value optimization)
    bool nrvo_can;                      // true means we can do it
    VarDeclaration *nrvo_var;           // variable to replace with shidden
#if IN_DMD
    Symbol *shidden;                    // hidden pointer passed to function
#endif

    ReturnStatements *returns;
    GotoStatements *gotos;              // Gotos with forward references

    BUILTIN builtin;               // set if this is a known, builtin
                                        // function we can evaluate at compile
                                        // time

    int tookAddressOf;                  // set if someone took the address of
                                        // this function
    bool requiresClosure;               // this function needs a closure
    VarDeclarations closureVars;        // local variables in this function
                                        // which are referenced by nested
                                        // functions
    FuncDeclarations siblingCallers;    // Sibling nested functions which
                                        // called this one
#if IN_DMD
    FuncDeclarations deferred;          // toObjFile() these functions after this one
#endif

    unsigned flags;
    #define FUNCFLAGpurityInprocess 1   // working on determining purity
    #define FUNCFLAGsafetyInprocess 2   // working on determining safety
    #define FUNCFLAGnothrowInprocess 4  // working on determining nothrow

    FuncDeclaration(Loc loc, Loc endloc, Identifier *id, StorageClass storage_class, Type *type);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void semantic2(Scope *sc);
    void semantic3(Scope *sc);
    bool functionSemantic();
    bool functionSemantic3();
    // called from semantic3
    VarDeclaration *declareThis(Scope *sc, AggregateDeclaration *ad);
    bool equals(RootObject *o);

    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    void bodyToCBuffer(OutBuffer *buf, HdrGenState *hgs);
    int overrides(FuncDeclaration *fd);
    int findVtblIndex(Dsymbols *vtbl, int dim);
    bool overloadInsert(Dsymbol *s);
    FuncDeclaration *overloadExactMatch(Type *t);
    TemplateDeclaration *findTemplateDeclRoot();
    MATCH leastAsSpecialized(FuncDeclaration *g);
    LabelDsymbol *searchLabel(Identifier *ident);
    AggregateDeclaration *isThis();
    AggregateDeclaration *isMember2();
    int getLevel(Loc loc, Scope *sc, FuncDeclaration *fd); // lexical nesting level difference
    void appendExp(Expression *e);
    void appendState(Statement *s);
    const char *mangle(bool isv = false);
    const char *mangleExact(bool isv = false);
    const char *toPrettyChars();
    const char *toFullSignature();  // for diagnostics, e.g. 'int foo(int x, int y) pure'
    bool isMain();
    bool isWinMain();
    bool isDllMain();
    BUILTIN isBuiltin();
    bool isExport();
    bool isImportedSymbol();
    bool isCodeseg();
    bool isOverloadable();
    bool hasOverloads();
    PURE isPure();
    PURE isPureBypassingInference();
    bool setImpure();
    bool isSafe();
    bool isSafeBypassingInference();
    bool isTrusted();
    bool setUnsafe();
    bool isolateReturn();
    bool parametersIntersect(Type *t);
    virtual bool isNested();
    bool needThis();
    bool isVirtualMethod();
    virtual bool isVirtual();
    virtual bool isFinalFunc();
    virtual bool addPreInvariant();
    virtual bool addPostInvariant();
    Expression *interpret(InterState *istate, Expressions *arguments, Expression *thisexp = NULL);
    void ctfeCompile();
    void inlineScan();
    int canInline(int hasthis, int hdrscan, int statementsToo);
    Expression *expandInline(InlineScanState *iss, Expression *eret, Expression *ethis, Expressions *arguments, Statement **ps);
    const char *kind();
    void toDocBuffer(OutBuffer *buf, Scope *sc);
    FuncDeclaration *isUnique();
    void checkNestedReference(Scope *sc, Loc loc);
    bool needsClosure();
    bool hasNestedFrameRefs();
    void buildResultVar();
    Statement *mergeFrequire(Statement *, Expressions *params = 0);
    Statement *mergeFensure(Statement *, Identifier *oid, Expressions *params = 0);
    Parameters *getParameters(int *pvarargs);

// LDC: give argument types to runtime functions
    static FuncDeclaration *genCfunc(Parameters *args, Type *treturn, const char *name);
    static FuncDeclaration *genCfunc(Parameters *args, Type *treturn, Identifier *id);

#if IN_DMD
    Symbol *toSymbol();
    Symbol *toThunkSymbol(int offset);  // thunk version
    void toObjFile(int multiobj);                       // compile to .obj file
    int cvMember(unsigned char *p);
    void buildClosure(IRState *irs); // Should this be inside or outside the #if IN_DMD?
#endif
    bool needsCodegen();
    FuncDeclaration *isFuncDeclaration() { return this; }

    virtual FuncDeclaration *toAliasFunc() { return this; }

#if IN_LLVM
    IrFuncTy irFty;

    std::string intrinsicName;
    uint32_t priority;

    bool isIntrinsic();
    bool isVaIntrinsic();

    // we keep our own table of label statements as LabelDsymbolS
    // don't always carry their corresponding statement along ...
    typedef std::map<const char*, LabelStatement*> LabelMap;
    LabelMap labmap;

    // Functions that wouldn't have gotten semantic3'ed if we weren't inlining set this flag.
    bool availableExternally;

    // true if overridden with the pragma(LDC_allow_inline); stmt
    bool allowInlining;

    // true if set with the pragma(LDC_never_inline); stmt
    bool neverInline;

    // true if has inline assembler
    bool inlineAsm;

    // CALYPSO
    virtual void toResolveFunction();
#endif

    void accept(Visitor *v) { v->visit(this); }
};

FuncDeclaration *resolveFuncCall(Loc loc, Scope *sc, Dsymbol *s,
        Objects *tiargs,
        Type *tthis,
        Expressions *arguments,
        int flags = 0);

class FuncAliasDeclaration : public FuncDeclaration
{
public:
    FuncDeclaration *funcalias;
    bool hasOverloads;

    FuncAliasDeclaration(FuncDeclaration *funcalias, bool hasOverloads = true);

    FuncAliasDeclaration *isFuncAliasDeclaration() { return this; }
    const char *kind();
#if IN_DMD
    Symbol *toSymbol();
#endif
    const char *mangle(bool isv = false);

    FuncDeclaration *toAliasFunc();
    void accept(Visitor *v) { v->visit(this); }
};

class FuncLiteralDeclaration : public FuncDeclaration
{
public:
    TOK tok;                       // TOKfunction or TOKdelegate
    Type *treq;                         // target of return type inference

    FuncLiteralDeclaration(Loc loc, Loc endloc, Type *type, TOK tok,
        ForeachStatement *fes, Identifier *id = NULL);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    Dsymbol *syntaxCopy(Dsymbol *);
    bool isNested();
    bool isVirtual();

    FuncLiteralDeclaration *isFuncLiteralDeclaration() { return this; }
    const char *kind();

#if IN_LLVM
    // If this is only used as alias parameter to a template instantiation,
    // keep track of which one, as the function will only be codegen'ed in the
    // module the template instance is pushed to, which is not always the same
    // as this->module because of the importedFrom check in
    // TemplateInstance::semantic and the fact that importedFrom is only set
    // once for the first module.
    TemplateInstance *owningTemplate;
#endif

    const char *toPrettyChars();
    void accept(Visitor *v) { v->visit(this); }
};

class CtorDeclaration : public FuncDeclaration
{
public:
    CtorDeclaration(Loc loc, Loc endloc, StorageClass stc, Type *type);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    const char *kind();
    char *toChars();
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();

    CtorDeclaration *isCtorDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class PostBlitDeclaration : public FuncDeclaration
{
public:
    PostBlitDeclaration(Loc loc, Loc endloc, StorageClass stc, Identifier *id);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();
    bool overloadInsert(Dsymbol *s);
    void emitComment(Scope *sc);

    PostBlitDeclaration *isPostBlitDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class DtorDeclaration : public FuncDeclaration
{
public:
    DtorDeclaration(Loc loc, Loc endloc);
    DtorDeclaration(Loc loc, Loc endloc, StorageClass stc, Identifier *id);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    const char *kind();
    char *toChars();
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();
    bool overloadInsert(Dsymbol *s);
    void emitComment(Scope *sc);

    DtorDeclaration *isDtorDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class StaticCtorDeclaration : public FuncDeclaration
{
public:
    StaticCtorDeclaration(Loc loc, Loc endloc);
    StaticCtorDeclaration(Loc loc, Loc endloc, const char *name);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    AggregateDeclaration *isThis();
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();
    bool hasStaticCtorOrDtor();
    void emitComment(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    StaticCtorDeclaration *isStaticCtorDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class SharedStaticCtorDeclaration : public StaticCtorDeclaration
{
public:
    SharedStaticCtorDeclaration(Loc loc, Loc endloc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    SharedStaticCtorDeclaration *isSharedStaticCtorDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class StaticDtorDeclaration : public FuncDeclaration
{
public:
    VarDeclaration *vgate;      // 'gate' variable

    StaticDtorDeclaration(Loc loc, Loc endloc, StorageClass stc);
    StaticDtorDeclaration(Loc loc, Loc endloc, const char *name, StorageClass stc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    AggregateDeclaration *isThis();
    bool isVirtual();
    bool hasStaticCtorOrDtor();
    bool addPreInvariant();
    bool addPostInvariant();
    void emitComment(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    StaticDtorDeclaration *isStaticDtorDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class SharedStaticDtorDeclaration : public StaticDtorDeclaration
{
public:
    SharedStaticDtorDeclaration(Loc loc, Loc endloc, StorageClass stc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    SharedStaticDtorDeclaration *isSharedStaticDtorDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class InvariantDeclaration : public FuncDeclaration
{
public:
    InvariantDeclaration(Loc loc, Loc endloc, StorageClass stc, Identifier *id = NULL);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();
    void emitComment(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    InvariantDeclaration *isInvariantDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class UnitTestDeclaration : public FuncDeclaration
{
public:
    char *codedoc; /** For documented unittest. */
    UnitTestDeclaration(Loc loc, Loc endloc, char *codedoc);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    AggregateDeclaration *isThis();
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();
    void emitComment(Scope *sc);
    void inlineScan();
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);

    UnitTestDeclaration *isUnitTestDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

class NewDeclaration : public FuncDeclaration
{
public:
    Parameters *arguments;
    int varargs;

    NewDeclaration(Loc loc, Loc endloc, Parameters *arguments, int varargs);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    const char *kind();
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();

    NewDeclaration *isNewDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};


class DeleteDeclaration : public FuncDeclaration
{
public:
    Parameters *arguments;

    DeleteDeclaration(Loc loc, Loc endloc, Parameters *arguments);
    Dsymbol *syntaxCopy(Dsymbol *);
    void semantic(Scope *sc);
    void toCBuffer(OutBuffer *buf, HdrGenState *hgs);
    const char *kind();
    bool isDelete();
    bool isVirtual();
    bool addPreInvariant();
    bool addPostInvariant();
    DeleteDeclaration *isDeleteDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

#endif /* DMD_DECLARATION_H */
