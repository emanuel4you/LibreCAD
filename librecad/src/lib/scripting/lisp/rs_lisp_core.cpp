/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 emanuel

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifdef DEVELOPER

#include "rs_python.h"
#include "rs_lisp.h"

#include "rs_lisp_lcl.h"
#include "rs_lisp_env.h"
#include "rs_lisp_staticlist.h"
#include "rs_lisp_types.h"
#include "rs_lisp_main.h"

#include "lc_defaults.h"

#include "rs_scriptingapi.h"
#include "rs.h"
#include "rs_scripting_inputhandle.h"
#include "rs_eventhandler.h"
#include "rs_filterdxfrw.h"
#include "rs_graphicview.h"
#include "rs_entity.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_text.h"
#include "rs_solid.h"
#include "rs_layer.h"
#include "rs_fontlist.h"
#include "qg_actionhandler.h"

#ifndef _WIN32
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#endif

#include <stdio.h>
#include <math.h>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <chrono>
#include <ctime>
#include <cmath>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <random>

/* temp defined */
#include <regex>

unsigned int tmpFileCount = 0;

typedef std::regex Regex;
static const Regex intRegex("[+-]?[0-9]+|[+-]?0[xX][0-9A-Fa-f]");
static const Regex floatRegex("[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?");
static const Regex floatPointRegex("[.]{1}\\d+$");

static lclValuePtr entget(lclEname *en);
static lclValuePtr getvar(const std::string &sysvar);
bool getApiData(lclValueVec* items, RS_ScriptingApiData &apiData);

#define CHECK_ARGS_IS(expected) \
    checkArgsIs(name.c_str(), expected, \
                  std::distance(argsBegin, argsEnd))

#define CHECK_ARGS_BETWEEN(min, max) \
    checkArgsBetween(name.c_str(), min, max, \
                       std::distance(argsBegin, argsEnd))

#define CHECK_ARGS_AT_LEAST(expected) \
    checkArgsAtLeast(name.c_str(), expected, \
                        std::distance(argsBegin, argsEnd))

#define FLOAT_PTR \
    (argsBegin->ptr()->type() == LCLTYPE::REAL)

#define INT_PTR \
    (argsBegin->ptr()->type() == LCLTYPE::INT)

#define STR_PTR \
    (argsBegin->ptr()->type() == LCLTYPE::STR)

#define LIST_PTR \
    (argsBegin->ptr()->type() == LCLTYPE::LIST)

#define NIL_PTR \
    (argsBegin->ptr()->print(true) == "nil")

#define TRUE_PTR \
    (argsBegin->ptr()->print(true) == "true")

#define T_PTR \
    (argsBegin->ptr()->print(true) == "T")

#define FALSE_PTR \
    (argsBegin->ptr()->print(true) == "false")

bool argsHasFloat(lclValueIter argsBegin, lclValueIter argsEnd)
{
    for (auto it = argsBegin; it != argsEnd; ++it) {
        if (it->ptr()->type() == LCLTYPE::REAL) {
            return true;
        }
    }
    return false;
}

#define ARGS_HAS_FLOAT \
    argsHasFloat(argsBegin, argsEnd)

#define AG_INT(name) \
    CHECK_IS_NUMBER(argsBegin->ptr()) \
    lclInteger* name = VALUE_CAST(lclInteger, *argsBegin++)

#define ADD_INT_VAL(val) \
    CHECK_IS_NUMBER(argsBegin->ptr()) \
    lclInteger val = dynamic_cast<lclInteger*>(argsBegin->ptr());

#define ADD_FLOAT_VAL(val) \
    CHECK_IS_NUMBER(argsBegin->ptr()) \
    lclDouble val = dynamic_cast<lclDouble*>(argsBegin->ptr());

#define ADD_LIST_VAL(val) \
    lclList val = dynamic_cast<lclList*>(argsBegin->ptr());

#define SET_INT_VAL(opr, checkDivByZero) \
    ADD_INT_VAL(*intVal) \
    intValue = intValue opr intVal->value(); \
    if (checkDivByZero) { \
        LCL_CHECK(intVal->value() != 0, "Division by zero"); }

#define SET_FLOAT_VAL(opr, checkDivByZero) \
    if (FLOAT_PTR) \
    { \
        ADD_FLOAT_VAL(*floatVal) \
        floatValue = floatValue opr floatVal->value(); \
        if (checkDivByZero) { \
            LCL_CHECK(floatVal->value() != 0.0, "Division by zero"); } \
    } \
    else \
    { \
        ADD_INT_VAL(*intVal) \
        floatValue = floatValue opr double(intVal->value()); \
        if (checkDivByZero) { \
            LCL_CHECK(intVal->value() != 0, "Division by zero"); } \
    }

static String printValues(lclValueIter begin, lclValueIter end,
                           const String& sep, bool readably);

static int countValues(lclValueIter begin, lclValueIter end);

static StaticList<lclBuiltIn*> handlers;

#define ARG(type, name) type* name = VALUE_CAST(type, *argsBegin++)

#define FUNCNAME(uniq) builtIn ## uniq
#define HRECNAME(uniq) handler ## uniq
#define BUILTIN_DEF(uniq, symbol) \
    static lclBuiltIn::ApplyFunc FUNCNAME(uniq); \
    static StaticList<lclBuiltIn*>::Node HRECNAME(uniq) \
        (handlers, new lclBuiltIn(symbol, FUNCNAME(uniq))); \
    lclValuePtr FUNCNAME(uniq)(const String& name, \
        lclValueIter argsBegin, lclValueIter argsEnd)

#define BUILTIN(symbol)  BUILTIN_DEF(__LINE__, symbol)

#define BUILIN_ALIAS(uniq) \
    FUNCNAME(uniq)(name, argsBegin, argsEnd)

#define BUILTIN_ISA(symbol, type) \
    BUILTIN(symbol) { \
        CHECK_ARGS_IS(1); \
        return lcl::boolean(DYNAMIC_CAST(type, *argsBegin)); \
    }

#define BUILTIN_IS(op, constant) \
    BUILTIN(op) { \
        CHECK_ARGS_IS(1); \
        return lcl::boolean(*argsBegin == lcl::constant()); \
    }

#define BUILTIN_INTOP(op, checkDivByZero) \
    BUILTIN(#op) { \
        BUILTIN_VAL(op, checkDivByZero); \
        }

#define BUILTIN_VAL(opr, checkDivByZero) \
    int args = CHECK_ARGS_AT_LEAST(0); \
    if (args == 0) { \
        return lcl::integer(0); \
    } \
    if (args == 1) { \
        if (FLOAT_PTR) { \
            ADD_FLOAT_VAL(*floatValue) \
            return lcl::ldouble(floatValue->value()); \
        } else { \
            ADD_INT_VAL(*intValue) \
            return lcl::integer(intValue->value()); \
        } \
    } \
    if (ARGS_HAS_FLOAT) { \
        BUILTIN_FLOAT_VAL(opr, checkDivByZero) \
    } else { \
        BUILTIN_INT_VAL(opr, checkDivByZero) \
    }

#define BUILTIN_FLOAT_VAL(opr, checkDivByZero) \
    [[maybe_unused]] double floatValue = 0; \
    SET_FLOAT_VAL(+, false); \
    argsBegin++; \
    do { \
        SET_FLOAT_VAL(opr, checkDivByZero); \
        argsBegin++; \
    } while (argsBegin != argsEnd); \
    return lcl::ldouble(floatValue);

#define BUILTIN_INT_VAL(opr, checkDivByZero) \
    [[maybe_unused]] int intValue = 0; \
    SET_INT_VAL(+, false); \
    argsBegin++; \
    do { \
        SET_INT_VAL(opr, checkDivByZero); \
        argsBegin++; \
    } while (argsBegin != argsEnd); \
    return lcl::integer(intValue);

#define BUILTIN_FUNCTION(foo) \
    CHECK_ARGS_IS(1); \
    if (FLOAT_PTR) { \
        ADD_FLOAT_VAL(*lhs) \
        return lcl::ldouble(foo(lhs->value())); } \
    else { \
        ADD_INT_VAL(*lhs) \
        return lcl::ldouble(foo(lhs->value())); }

#define BUILTIN_OP_COMPARE(opr) \
    CHECK_ARGS_IS(2); \
    if (((argsBegin->ptr()->type() == LCLTYPE::LIST) && ((argsBegin + 1)->ptr()->type() == LCLTYPE::LIST)) || \
        ((argsBegin->ptr()->type() == LCLTYPE::VEC) && ((argsBegin + 1)->ptr()->type() == LCLTYPE::VEC))) { \
        ARG(lclSequence, lhs); \
        ARG(lclSequence, rhs); \
        return lcl::boolean(lhs->count() opr rhs->count()); } \
    if (ARGS_HAS_FLOAT) { \
        if (FLOAT_PTR) { \
            ADD_FLOAT_VAL(*floatLhs) \
            argsBegin++; \
            if (FLOAT_PTR) { \
                ADD_FLOAT_VAL(*floatRhs) \
                return lcl::boolean(floatLhs->value() opr floatRhs->value()); } \
            else { \
               ADD_INT_VAL(*intRhs) \
               return lcl::boolean(floatLhs->value() opr double(intRhs->value())); } } \
        else { \
            ADD_INT_VAL(*intLhs) \
            argsBegin++; \
            ADD_FLOAT_VAL(*floatRhs) \
            return lcl::boolean(double(intLhs->value()) opr floatRhs->value()); } } \
    else { \
        ADD_INT_VAL(*intLhs) \
        argsBegin++; \
        ADD_INT_VAL(*intRhs) \
        return lcl::boolean(intLhs->value() opr intRhs->value()); }

// helper foo to cast integer (64 bit) type to char (8 bit) type
unsigned char itoa64(const int &sign)
{
    int bit64[8];
    unsigned char result = 0;

    if(sign < 0)
    {
        std::cout << "Warning: out of char value!" << std::endl;
        return result;
    }

    for (int i = 0; i < 8; i++)
    {
        bit64[i] = (sign >> i) & 1;
        if (bit64[i])
        {
            result |= 1 << i;
        }
    }
    return result;
}

bool compareNat(const String& a, const String& b)
{
    //std::cout << a << " " << b << std::endl;
    if (a.empty()) {
        return true;
    }
    if (b.empty()) {
        return false;
    }
    if (std::isdigit(a[0], std::locale()) && !std::isdigit(b[0], std::locale())) {
        return false;
    }
    if (!std::isdigit(a[0], std::locale()) && std::isdigit(b[0], std::locale())) {
        return false;
    }
    if (!std::isdigit(a[0], std::locale()) && !std::isdigit(b[0], std::locale())) {
        //std::cout << "HIT no dig" << std::endl;
        if (a[0] == '.' &&
            b[0] == '.' &&
            a.size() > 1 &&
            b.size() > 1) {
            return (std::toupper(a[1], std::locale()) < std::toupper(b[1], std::locale()));
        }

        if (std::toupper(a[0], std::locale()) == std::toupper(b[0], std::locale())) {
            return compareNat(a.substr(1), b.substr(1));
        }
        return (std::toupper(a[0], std::locale()) < std::toupper(b[0], std::locale()));
    }

    // Both strings begin with digit --> parse both numbers
    std::istringstream issa(a);
    std::istringstream issb(b);
    int ia, ib;
    issa >> ia;
    issb >> ib;
    if (ia != ib)
        return ia < ib;

    // Numbers are the same --> remove numbers and recurse
    String anew, bnew;
    std::getline(issa, anew);
    std::getline(issb, bnew);
    return (compareNat(anew, bnew));
}

bool compareNatPath(const std::filesystem::path& a, const std::filesystem::path& b)
{
    return compareNat(a.string(), b.string());
}

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

BUILTIN_ISA("atom?",        lclAtom);
BUILTIN_ISA("double?",      lclDouble);
BUILTIN_ISA("file?",        lclFile);
BUILTIN_ISA("integer?",     lclInteger);
BUILTIN_ISA("keyword?",     lclKeyword);
BUILTIN_ISA("list?",        lclList);
BUILTIN_ISA("map?",         lclHash);
BUILTIN_ISA("sequential?",  lclSequence);
BUILTIN_ISA("string?",      lclString);
BUILTIN_ISA("symbol?",      lclSymbol);
BUILTIN_ISA("vector?",      lclVector);

BUILTIN_INTOP(+,            false);
BUILTIN_INTOP(/,            true);
BUILTIN_INTOP(*,            false);

BUILTIN_IS("true?",         trueValue);
BUILTIN_IS("false?",        falseValue);
BUILTIN_IS("nil?",          nilValue);

BUILTIN("-")
{
    int args = CHECK_ARGS_AT_LEAST(0);

    if (args == 0)
    {
        return lcl::integer(0);
    }

    if (args == 1)
    {
        if (FLOAT_PTR)
        {
            ADD_FLOAT_VAL(*lhs)
            return lcl::ldouble(-lhs->value());
        }
        else
        {
            ADD_INT_VAL(*lhs)
            return lcl::integer(-lhs->value());
        }
    }

    if (ARGS_HAS_FLOAT)
    {
        BUILTIN_FLOAT_VAL(-, false);
    }
    else
    {
        BUILTIN_INT_VAL(-, false);
    }
}

BUILTIN("%")
{
    CHECK_ARGS_AT_LEAST(2);
    if (ARGS_HAS_FLOAT) {
        return lcl::nilValue();
    } else {
        BUILTIN_INT_VAL(%, false);
    }
}

BUILTIN("<=")
{
    BUILTIN_OP_COMPARE(<=);
}

BUILTIN(">=")
{
    BUILTIN_OP_COMPARE(>=);
}

BUILTIN("<")
{
    BUILTIN_OP_COMPARE(<);
}

BUILTIN(">")
{
    BUILTIN_OP_COMPARE(>);
}

BUILTIN("=")
{
    CHECK_ARGS_IS(2);
    const lclValue* lhs = (*argsBegin++).ptr();
    const lclValue* rhs = (*argsBegin++).ptr();

    return lcl::boolean(lhs->isEqualTo(rhs));
}

BUILTIN("/=")
{
    CHECK_ARGS_IS(2);
    const lclValue* lhs = (*argsBegin++).ptr();
    const lclValue* rhs = (*argsBegin++).ptr();

    return lcl::boolean(!lhs->isEqualTo(rhs));
}

BUILTIN("~ ")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        return lcl::nilValue();
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(~lhs->value());
    }
}

BUILTIN("1+")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(lhs->value()+1);
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(lhs->value()+1);
    }
}

BUILTIN("1-")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(lhs->value()-1);
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(lhs->value()-1);
    }
}

BUILTIN("abs")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(abs(lhs->value()));
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(abs(lhs->value()));
    }
}

BUILTIN("acad_colordlg")
{
    int args = CHECK_ARGS_BETWEEN(1, 2);
    AG_INT(color);
    bool by = true;

    if (args == 2 && NIL_PTR)
    {
        by = false;
    }

    int result;
    return RS_SCRIPTINGAPI->colorDialog(color->value(),
                                        by,
                                        result) ? lcl::integer(result) : lcl::nilValue();
}

BUILTIN("acad_truecolordlg")
{
    int args = CHECK_ARGS_BETWEEN(1, 3);
    int tcolor = -1, color = -1, tbycolor = -1, bycolor = -1;
    bool allowbylayer = true;
    ARG(lclSequence, seq);

    if(!seq->isDotted())
        return lcl::nilValue();

    const lclInteger* gc = DYNAMIC_CAST(lclInteger, seq->item(0));
    const lclInteger* c = DYNAMIC_CAST(lclInteger, seq->item(2));

    if(gc->value() == 52)
        color = c->value();

    if(gc->value() == 420)
        tcolor = c->value();

    if (args >= 2 && NIL_PTR)
    {
        allowbylayer = false;
    }

    if (args == 3)
    {
        ARG(lclSequence, byColor);
        const lclInteger* bygc = DYNAMIC_CAST(lclInteger, byColor->item(0));
        const lclInteger* byc = DYNAMIC_CAST(lclInteger, byColor->item(2));

        if(bygc->value() == 52)
            bycolor = byc->value();

        if(byc->value() == 420)
            tbycolor = byc->value();
    }

    int result, tresult;
    bool take = RS_SCRIPTINGAPI->trueColorDialog(tresult,
                                                 result,
                                                 tcolor,
                                                 color,
                                                 allowbylayer,
                                                 tbycolor,
                                                 bycolor
                                                 );

    if (take)
    {
        lclValueVec* list = new lclValueVec(0);

        if (result != -1)
        {
            lclValueVec* dotted = new lclValueVec(3);
            dotted->at(0) = new lclInteger(52);
            dotted->at(1) = new lclSymbol(".");
            dotted->at(2) = new lclInteger(result);
            list->push_back(new lclList(dotted));
        }

        if (tresult != -1)
        {
            lclValueVec* tdotted = new lclValueVec(3);
            tdotted->at(0) = new lclInteger(420);
            tdotted->at(1) = new lclSymbol(".");
            tdotted->at(2) = new lclInteger(tresult);
            list->push_back(new lclList(tdotted));
        }

        return lcl::list(list);
    }

    return lcl::nilValue();
}

BUILTIN("action_tile") {
    CHECK_ARGS_IS(2);
    ARG(lclString, id);
    ARG(lclString, action);

    return RS_SCRIPTINGAPI->actionTile(id->value().c_str(),
                                       action->value().c_str()) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("add_list")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, val);

    std::string result;
    return RS_SCRIPTINGAPI->addList(val->value().c_str(),
                                    result) ? lcl::string(result) : lcl::nilValue();
}

BUILTIN("alert")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, msg);

    RS_SCRIPTINGAPI->msgInfo(msg->value().c_str());
    return lcl::nilValue();
}

BUILTIN("angle")
{
    CHECK_ARGS_IS(2);
    ARG(lclSequence, pnt1);
    ARG(lclSequence, pnt2);
    double x1, x2, y1, y2;

    CHECK_IS_NUMBER(pnt1->item(0))
    CHECK_IS_NUMBER(pnt1->item(1))
    CHECK_IS_NUMBER(pnt2->item(1))
    CHECK_IS_NUMBER(pnt2->item(1))

    if (pnt1->item(0)->type() == LCLTYPE::INT)
    {
        const lclInteger* intX = VALUE_CAST(lclInteger, pnt1->item(0));
        x1 = double(intX->value());
    }
    else
    {
        const lclDouble* floatX = VALUE_CAST(lclDouble, pnt1->item(0));
        x1 = floatX->value();
    }

    if (pnt1->item(1)->type() == LCLTYPE::INT)
    {
        const lclInteger* intY = VALUE_CAST(lclInteger, pnt1->item(1));
        y1 = double(intY->value());
    }
    else
    {
        const lclDouble* floatY = VALUE_CAST(lclDouble, pnt1->item(1));
        y1 = floatY->value();
    }

    if (pnt2->item(0)->type() == LCLTYPE::INT)
    {
        const lclInteger* intX = VALUE_CAST(lclInteger, pnt2->item(0));
        x2 = double(intX->value());
    }
    else
    {
        const lclDouble* floatX = VALUE_CAST(lclDouble, pnt2->item(0));
        x2 = floatX->value();
    }

    if (pnt2->item(1)->type() == LCLTYPE::INT)
    {
        const lclInteger* intY = VALUE_CAST(lclInteger, pnt2->item(1));
        y2 = double(intY->value());
    }
    else
    {
        const lclDouble* floatY = VALUE_CAST(lclDouble, pnt2->item(1));
        y2 = floatY->value();
    }

    return lcl::ldouble(std::atan2(y2 - y1, x2 - x1));
}

BUILTIN("apply")
{
    CHECK_ARGS_AT_LEAST(2);
    lclValuePtr op = *argsBegin++; // this gets checked in APPLY

    // both LISPs
    if (op->type() == LCLTYPE::SYM ||
        op->type() == LCLTYPE::LIST) {
        op = EVAL(op, NULL);
    }

    // Copy the first N-1 arguments in.
    lclValueVec args(argsBegin, argsEnd-1);

    // Then append the argument as a list.
    const lclSequence* lastArg = VALUE_CAST(lclSequence, *(argsEnd-1));
    const int length = lastArg->count();
    for (int i = 0; i < length; i++) {
        args.push_back(lastArg->item(i));
    }

    return APPLY(op, args.begin(), args.end());
}

BUILTIN("ascii")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;

    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
    {
        return lcl::integer(int(s->value().c_str()[0]));
    }

    return lcl::integer(0);
}

BUILTIN("assoc")
{
    CHECK_ARGS_AT_LEAST(1);
    //both LISPs
    if (!(argsBegin->ptr()->type() == LCLTYPE::MAP)) {
        lclValuePtr op = *argsBegin++;
        ARG(lclSequence, seq);

        const int length = seq->count();
        lclValueVec* items = new lclValueVec(length);
        std::copy(seq->begin(), seq->end(), items->begin());

        for (int i = 0; i < length; i++) {
            if (items->at(i)->type() == LCLTYPE::LIST) {
                lclList* list = VALUE_CAST(lclList, items->at(i));
                if (list->count() == 2) {
                    lclValueVec* duo = new lclValueVec(2);
                    std::copy(list->begin(), list->end(), duo->begin());
                    if (duo->begin()->ptr()->print(true) == op->print(true)) {
                        return list;
                    }
                }
                if (list->count() == 3) {
                    lclValueVec* dotted = new lclValueVec(3);
                    std::copy(list->begin(), list->end(), dotted->begin());
                    if (dotted->begin()->ptr()->print(true) == op->print(true)
                        && (dotted->at(1)->print(true) == ".")
                    ) {
                        return list;
                    }
                }
            }
        }
        return lcl::nilValue();
    }
    ARG(lclHash, hash);

    return hash->assoc(argsBegin, argsEnd);
}

BUILTIN("atan")
{
    BUILTIN_FUNCTION(atan);
}

BUILTIN("atof")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;

    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
    {
        if(std::regex_match(s->value().c_str(), intRegex) ||
            std::regex_match(s->value().c_str(), floatRegex))
            {
                return lcl::ldouble(atof(s->value().c_str()));
            }
    }
    return lcl::ldouble(0);
}

BUILTIN("atoi")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;

    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
    {
        if (std::regex_match(s->value().c_str(), intRegex))
        {
            return lcl::integer(atoi(s->value().c_str()));
        }
        if (std::regex_match(s->value().c_str(), floatRegex))
        {
            return lcl::integer(atoi(std::regex_replace(s->value().c_str(),
                                                        floatPointRegex, "").c_str()));
        }
    }
    return lcl::integer(0);
}

BUILTIN("atom")
{
    CHECK_ARGS_IS(1);

    return lcl::atom(*argsBegin);
}

BUILTIN("boolean?")
{
    CHECK_ARGS_IS(1);
    {
        return lcl::boolean(argsBegin->ptr()->type() == LCLTYPE::BOOLEAN);
    }
}

BUILTIN("bound?")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, replEnv)->print(true) == "nil")
    {
        return lcl::falseValue();
    }
    else
    {
        const lclSymbol* sym = DYNAMIC_CAST(lclSymbol, *argsBegin);
        if(!sym)
        {
            return lcl::falseValue();
        }
        else
        {
            if (replEnv->get(sym->value()) == lcl::nilValue())
            {
                return lcl::falseValue();
            }
        }
    }
    return lcl::trueValue();
}

BUILTIN("boundp")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, replEnv)->print(true) == "nil")
    {
        return lcl::nilValue();
    }
    else
    {
        const lclSymbol* sym = DYNAMIC_CAST(lclSymbol, *argsBegin);
        if(!sym)
        {
            return lcl::nilValue();
        }
        else
        {
            if (replEnv->get(sym->value()) == lcl::nilValue())
            {
                return lcl::nilValue();
            }
        }
    }
    return lcl::trueValue();
}

BUILTIN("car")
{
    CHECK_ARGS_IS(1);
#if 0
    // clojure?
    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }
#endif
    ARG(lclSequence, seq);
    if(!seq->count())
    {
        return lcl::nilValue();
    }

    return seq->first();
}

#if 0
BUILTIN("cadr")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    if(0 < seq->count())
    {
        return lcl::nilValue();
    }

    return seq->item(1);
}

BUILTIN("caddr")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    LCL_CHECK(2 < seq->count(), "Index out of range");

    return seq->item(2);
}
#endif

BUILTIN("cdr")
{
    CHECK_ARGS_IS(1);

    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }

    ARG(lclSequence, seq);

    if(!seq->count())
    {
        return lcl::nilValue();
    }

    if (seq->isDotted()) {
            return seq->dotted();
    }

    return seq->rest();
}

BUILTIN("chr")
{
    CHECK_ARGS_IS(1);
    unsigned char sign = 0;

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        auto sign64 = static_cast<int>(lhs->value());
        sign = itoa64(sign64);
    }
    else
    {
        ADD_INT_VAL(*lhs)
        sign = itoa64(lhs->value());
    }

    return lcl::string(String(1 , sign));
}

BUILTIN("close")
{
    CHECK_ARGS_IS(1);
    ARG(lclFile, pf);

    return pf->close();
}

BUILTIN("command")
{
    CHECK_ARGS_AT_LEAST(1);
    QString cmd = "";

    for (auto it = argsBegin; it != argsEnd; ++it)
    {
        switch(it->ptr()->type())
        {
            case LCLTYPE::STR:
            {
                const lclString* s = DYNAMIC_CAST(lclString,*it);
                cmd = s->value().c_str();
                RS_SCRIPTINGAPI->command(cmd);
                cmd = "";
            }
                break;
            case LCLTYPE::LIST:
            {
                const lclList* l = DYNAMIC_CAST(lclList, *it);
                const int length = l->count();

                for (int i = 0; i < length; i++)
                {
                    cmd += l->item(i)->print(true).c_str();
                    if (i < length -1)
                    {
                        cmd += ",";
                    }
                }

                RS_SCRIPTINGAPI->command(cmd);
                cmd = "";
            }
                break;

            case LCLTYPE::INT:
            case LCLTYPE::REAL:
            {
                cmd += it->ptr()->print(true).c_str();
                RS_SCRIPTINGAPI->command(cmd);
                cmd = "";
            }
                break;
            default:
            {
                cmd = "";
                RS_SCRIPTINGAPI->command(cmd);
            }
                break;
        }
        //std::cout << "parameter: " << it->ptr()->print(true) << " type: " << (int)it->ptr()->type() << std::endl;
    }

    return lcl::nilValue();
}

BUILTIN("concat")
{
    Q_UNUSED(name);
    int count = 0;
    for (auto it = argsBegin; it != argsEnd; ++it) {
        const lclSequence* seq = VALUE_CAST(lclSequence, *it);
        const int length = seq->count();
        count += length;
    }

    lclValueVec* items = new lclValueVec(count);
    int offset = 0;
    for (auto it = argsBegin; it != argsEnd; ++it) {
        const lclSequence* seq = STATIC_CAST(lclSequence, *it);
        const int length = seq->count();
        std::copy(seq->begin(), seq->end(), items->begin() + offset);
        offset += length;
    }

    return lcl::list(items);
}
#if 1
BUILTIN("bla")
{
    CHECK_ARGS_IS(0);

    RS_Graphic *graphic = RS_SCRIPTINGAPI->getGraphic();

    if(!graphic)
    {
        return lcl::nilValue();
    }

    QHash<QString, RS_Variable>vars = graphic->getVariableDict();
    QHash<QString, RS_Variable>::iterator it = vars.begin();

    RS_Vector v;

    int n = 0;
    while (it != vars.end())
    {
        qDebug() << "[SYSVAR]:" << n++ << it.key();
#if 1
        switch (it.value().getType())
        {
        case RS2::VariableInt:
            qDebug() << "value:" << it.value().getInt();
            // it.value().getCode());
            break;
        case RS2::VariableDouble:
            qDebug() << "value:" << it.value().getDouble();
            // it.value().getCode());
            break;
        case RS2::VariableString:
            qDebug() << "value:" << it.value().getString();
            // it.value().getString().toUtf8().data(); it.value().getCode();
            break;
        case RS2::VariableVector:
            v = it.value().getVector();
            qDebug() << "value:" << v.x << "," << v.y << "," << v.z;
            // it.key().toStdString(), DRW_Coord(v.x, v.y, v.z); it.value().getCode());
            break;
        default:
            break;
        }
#endif
        ++it;
    }

    return lcl::nilValue();
    //return BUILIN_ALIAS(543);
    //return builtIn540(name, argsBegin, argsEnd);
}
#endif
BUILTIN("conj")
{
    CHECK_ARGS_AT_LEAST(1);
    ARG(lclSequence, seq);

    return seq->conj(argsBegin, argsEnd);
}

BUILTIN("cons")
{
    CHECK_ARGS_IS(2);
    lclValuePtr first = *argsBegin++;
    lclValuePtr second = *argsBegin;

    if (second->type() == LCLTYPE::INT ||
        second->type() == LCLTYPE::REAL ||
        second->type() == LCLTYPE::STR)
    {
        lclValueVec* items = new lclValueVec(3);
        items->at(0) = first;
        items->at(1) = new lclSymbol(".");
        items->at(2) = second;
        return lcl::list(items);
    }

    ARG(lclSequence, rest);

    lclValueVec* items = new lclValueVec(1 + rest->count());
    items->at(0) = first;
    std::copy(rest->begin(), rest->end(), items->begin() + 1);

    return lcl::list(items);
}

BUILTIN("contains?")
{
    CHECK_ARGS_IS(2);
    if (*argsBegin == lcl::nilValue()) {
        return *argsBegin;
    }
    ARG(lclHash, hash);
    return lcl::boolean(hash->contains(*argsBegin));
}

BUILTIN("copyright")
{
    CHECK_ARGS_IS(0);
    std::cout << RS_SCRIPTINGAPI->copyright();

    return lcl::nilValue();
}

BUILTIN("cos")
{
    BUILTIN_FUNCTION(cos);
}

BUILTIN("count")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::integer(0);
    }

    ARG(lclSequence, seq);
    return lcl::integer(seq->count());
}

BUILTIN("credits")
{
    CHECK_ARGS_IS(0);
    std::cout << RS_SCRIPTINGAPI->credits();
    return lcl::nilValue();
}

BUILTIN("deref")
{
    CHECK_ARGS_IS(1);
    ARG(lclAtom, atom);

    return atom->deref();
}

BUILTIN("dimx_tile")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);

    int x;
    return RS_SCRIPTINGAPI->dimxTile(key->value().c_str(), x) ? lcl::integer(x) : lcl::nilValue();
}

BUILTIN("dimy_tile")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);

    int y;
    return RS_SCRIPTINGAPI->dimyTile(key->value().c_str(), y) ? lcl::integer(y) : lcl::nilValue();
}

BUILTIN("dissoc")
{
    CHECK_ARGS_AT_LEAST(1);
    ARG(lclHash, hash);

    return hash->dissoc(argsBegin, argsEnd);
}

BUILTIN("done_dialog") {
    int args = CHECK_ARGS_BETWEEN(0, 1);
    int result = -1;
    int x, y;

    if (args == 1)
    {
        AG_INT(val);
        result = val->value();
    }

    if (RS_SCRIPTINGAPI->doneDialog(result, x, y))
    {
        lclValueVec* items = new lclValueVec(2);
        items->at(0) = lcl::integer(x);
        items->at(1) = lcl::integer(y);
        return lcl::list(items);
    }

    return lcl::nilValue();
}

BUILTIN("empty?")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    return lcl::boolean(seq->isEmpty());
}

BUILTIN("end_image")
{
    CHECK_ARGS_IS(0);

    RS_SCRIPTINGAPI->endImage();

    return lcl::nilValue();
}

BUILTIN("end_list")
{
    CHECK_ARGS_IS(0);

    RS_SCRIPTINGAPI->endList();

    return lcl::nilValue();
}

BUILTIN("entdel")
{
    CHECK_ARGS_IS(1);
    ARG(lclEname, en);

    return RS_SCRIPTINGAPI->entdel(en->value()) ? lcl::ename(en->value()) : lcl::nilValue();
}

BUILTIN("entget")
{
    CHECK_ARGS_IS(1);
    ARG(lclEname, en);

    return entget(en);
}

BUILTIN("entlast")
{
    CHECK_ARGS_IS(0);

    unsigned int id = RS_SCRIPTINGAPI->entlast();

    return id > 0 ? lcl::ename(id) : lcl::nilValue();
}

BUILTIN("entmake")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    const int length = seq->count();
    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());

    RS_ScriptingApiData apiData;

    if (!getApiData(items, apiData) || apiData.etype == "")
    {
        return lcl::nilValue();
    }

    if (RS_SCRIPTINGAPI->entmake(apiData))
    {
        lclEname *en = new lclEname(RS_SCRIPTINGAPI->entlast());
        return entget(en);
    }

    return lcl::nilValue();
}

BUILTIN("entmod")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    unsigned int entityId = 0;
    const int length = seq->count();

    for (int i = 0; i < length; i++)
    {
        const lclList *list = VALUE_CAST(lclList, seq->item(i));

        if (list->isDotted() && list->item(0)->print(true) == "-1")
        {
            const lclEname *ename = VALUE_CAST(lclEname, list->item(2));
            entityId = ename->value();
            break;
        }
    }

    if (entityId == 0)
    {
        return lcl::nilValue();
    }

    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());

    RS_ScriptingApiData apiData;
    RS_EntityContainer* entityContainer = RS_SCRIPTINGAPI->getContainer();

    if(entityContainer->count())
    {
        for (auto entity: *entityContainer)
        {
            if (entity->getId() == entityId)
            {
                apiData.pen = entity->getPen(false);

                if (!getApiData(items, apiData) || apiData.id.empty())
                {
                    return lcl::nilValue();
                }

                if (RS_SCRIPTINGAPI->entmod(entity, apiData))
                {
                    lclEname *en = new lclEname(apiData.id.front());
                    return entget(en);
                }
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("entnext")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    unsigned int id = 0;

    if (args == 0)
    {
        id = RS_SCRIPTINGAPI->entnext();
    }
    else
    {
        ARG(lclEname, en);
        id = RS_SCRIPTINGAPI->entnext(en->value());
    }

    return id > 0 ? lcl::ename(id) : lcl::nilValue();
}

BUILTIN("entsel")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    QString prompt = "";
    unsigned long id;
    RS_Vector result;

    if (args == 1 && !NIL_PTR)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    if (RS_SCRIPTINGAPI->entsel(Lisp_CommandEdit, QObject::tr(qUtf8Printable(prompt)), id, result))
    {
        lclValueVec *ptn = new lclValueVec(3);
        ptn->at(0) = lcl::ldouble(result.x);
        ptn->at(1) = lcl::ldouble(result.y);
        ptn->at(2) = lcl::ldouble(result.z);

        lclValueVec *res = new lclValueVec(2);
        res->at(0) = lcl::ename(id);
        res->at(1) = lcl::list(ptn);

        return lcl::list(res);
    }

    return lcl::nilValue();
}

BUILTIN("eval")
{
    CHECK_ARGS_IS(1);
    return EVAL(*argsBegin, NULL);
}

BUILTIN("exit")
{
    CHECK_ARGS_IS(0);

    throw -1;

    return lcl::nilValue();
}

BUILTIN("exp")
{
    BUILTIN_FUNCTION(exp);
}

BUILTIN("expt")
{
    CHECK_ARGS_IS(2);

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        argsBegin++;
        if (FLOAT_PTR)
        {
            ADD_FLOAT_VAL(*rhs)
            return lcl::ldouble(pow(lhs->value(),
                                    rhs->value()));
        }
        else
        {
            ADD_INT_VAL(*rhs)
            return lcl::ldouble(pow(lhs->value(),
                                    double(rhs->value())));
        }
    }
    else
    {
        ADD_INT_VAL(*lhs)
        argsBegin++;
        if (FLOAT_PTR)
        {
            ADD_FLOAT_VAL(*rhs)
            return lcl::ldouble(pow(double(lhs->value()),
                                    rhs->value()));
        }
        else
        {
            ADD_INT_VAL(*rhs)
            auto result = static_cast<int>(pow(double(lhs->value()),
                                    double(rhs->value())));
            return lcl::integer(result);
        }
    }
}

BUILTIN("fill_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x);
    AG_INT(y);
    AG_INT(width);
    AG_INT(height);
    AG_INT(color);

    return RS_SCRIPTINGAPI->fillImage(x->value(),
                                      y->value(),
                                      width->value(),
                                      height->value(),
                                      color->value()) ? lcl::integer(color->value()) : lcl::nilValue();
}

BUILTIN("first")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::nilValue();
    }
    ARG(lclSequence, seq);
    return seq->first();
}

BUILTIN("fix")
{
    CHECK_ARGS_IS(1);

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::integer(floor(lhs->value()));
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(lhs->value());
    }
}

BUILTIN("float")
{
    CHECK_ARGS_IS(1);

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(lhs->value());
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::ldouble(double(lhs->value()));
    }
}

BUILTIN("fn?")
{
    CHECK_ARGS_IS(1);
    lclValuePtr arg = *argsBegin++;

    // Lambdas are functions, unless they're macros.
    if (const lclLambda* lambda = DYNAMIC_CAST(lclLambda, arg)) {
        return lcl::boolean(!lambda->isMacro());
    }
    // Builtins are functions.
    return lcl::boolean(DYNAMIC_CAST(lclBuiltIn, arg));
}

BUILTIN("gcd")
{
    CHECK_ARGS_IS(2);
    AG_INT(first);
    AG_INT(second);

    return lcl::integer(std::gcd(first->value(), second->value()));
}

BUILTIN("get")
{
    CHECK_ARGS_IS(2);
    if (*argsBegin == lcl::nilValue()) {
        return *argsBegin;
    }
    ARG(lclHash, hash);
    return hash->get(*argsBegin);
}

BUILTIN("get_attr")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, key);
    ARG(lclString, attr);
    std::string result;

    return RS_SCRIPTINGAPI->getAttr(key->value().c_str(),
                                    attr->value().c_str(), result) ? lcl::string(result) : lcl::nilValue();
}

BUILTIN("get_tile")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);
    std::string result;

    return RS_SCRIPTINGAPI->getTile(key->value().c_str(),
                                    result) ? lcl::string(result) : lcl::nilValue();
}


BUILTIN("getangle")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString prompt = "";
    double x=0.0, y=0.0, z=0.0;
    double radius;
    bool ref = false;

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ref = true;
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    return RS_SCRIPTINGAPI->getAngle(Lisp_CommandEdit,
                                    qUtf8Printable(prompt),
                                    ref ? RS_Vector(x, y, z) : RS_Vector(),
                                    radius) ? lcl::ldouble(radius) : lcl::nilValue();
}

BUILTIN("getcorner")
{
    int args = CHECK_ARGS_BETWEEN(1, 2);
    QString prompt = "";
    double x=0.0, y=0.0, z=0.0;
    ARG(lclSequence, ptn);

    if (ptn->count() == 2)
    {
        if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
            (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT))
        {
            if (ptn->item(0)->type() == LCLTYPE::REAL)
            {
                const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                x = X->value();
            }
            if (ptn->item(0)->type() == LCLTYPE::INT)
            {
                const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                x = double(X->value());
            }
            if (ptn->item(1)->type() == LCLTYPE::REAL)
            {
                const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                y = Y->value();
            }
            if (ptn->item(1)->type() == LCLTYPE::INT)
            {
                const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                y = double(Y->value());
            }
        }
    }

    if (ptn->count() == 3)
    {
        if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
            (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
            (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
        {
            if (ptn->item(0)->type() == LCLTYPE::REAL)
            {
                const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                x = X->value();
            }
            if (ptn->item(0)->type() == LCLTYPE::INT)
            {
                const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                x = double(X->value());
            }
            if (ptn->item(1)->type() == LCLTYPE::REAL)
            {
                const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                y = Y->value();
            }
            if (ptn->item(1)->type() == LCLTYPE::INT)
            {
                const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                y = double(Y->value());
            }

            if (ptn->item(2)->type() == LCLTYPE::REAL)
            {
                const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                z = Z->value();
            }
            if (ptn->item(2)->type() == LCLTYPE::INT)
            {
                const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                z = double(Z->value());
            }
        }
    }

    if (args > 1)
    {
        ARG(lclString, msg);
        prompt = QObject::tr(msg->value().c_str());
    }

    RS_Vector result = RS_SCRIPTINGAPI->getCorner(Lisp_CommandEdit, qUtf8Printable(prompt), RS_Vector(x, y, z));

    if (result.valid)
    {
        lclValueVec *pt = new lclValueVec(3);
                    pt->at(0) = lcl::ldouble(result.x);
                    pt->at(1) = lcl::ldouble(result.y);
                    pt->at(2) = lcl::ldouble(result.z);

        return lcl::list(pt);
    }

    return lcl::nilValue();
}

BUILTIN("getdist")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString prompt = "";
    double x=0.0, y=0.0, z=0.0;
    double distance;
    bool ref = false;

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ref = true;
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }

        if ((!ref && args == 1) || (args == 2))
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }
    }

    return RS_SCRIPTINGAPI->getDist(Lisp_CommandEdit,
                                    qUtf8Printable(prompt),
                                    ref ? RS_Vector(x, y, z) : RS_Vector(),
                                    distance) ? lcl::ldouble(distance) : lcl::nilValue();
}

BUILTIN("getenv")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    if (const char* env_p = std::getenv(str->value().c_str())) {
        String env = env_p;
        return lcl::string(env);
    }
    return lcl::nilValue();
}

BUILTIN("getfiled")
{
    CHECK_ARGS_IS(4);
    ARG(lclString, title);
    ARG(lclString, def);
    ARG(lclString, ext);
    ARG(lclInteger, flags);
    String filename;

    return RS_SCRIPTINGAPI->getFiled(title->value().c_str(),
                                     def->value().c_str(),
                                     ext->value().c_str(),
                                     flags->value(),
                                     filename) ? lcl::string(filename) : lcl::nilValue();
}

BUILTIN("getint")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    int result;
    QString prompt = "";

    if (args == 1)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    return RS_SCRIPTINGAPI->getInteger(Lisp_CommandEdit,
                                    qUtf8Printable(prompt),
                                    result) ? lcl::integer(result) : lcl::nilValue();
}

BUILTIN("getkword") {
    int args = CHECK_ARGS_BETWEEN(0, 1);
    QString prompt = "";
    String result;

    if (args == 1)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    return RS_SCRIPTINGAPI->getKeyword(Lisp_CommandEdit,
                                      qUtf8Printable(prompt),
                                      result) ? lcl::string(result) : lcl::nilValue();

}

BUILTIN("getorient")
{
    /*
     * independent of the variables ANGDIR
     *
     * getangle to getorient
     * is only set as alias
     * nomaly depends of the variables ANGDIR
     * but is not implemented yet
     *
     */

    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString prompt = "";
    double x=0.0, y=0.0, z=0.0;
    double radius;
    bool ref = false;

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ref = true;
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    return RS_SCRIPTINGAPI->getOrient(Lisp_CommandEdit,
                                    qUtf8Printable(prompt),
                                    ref ? RS_Vector(x, y, z) : RS_Vector(),
                                    radius) ? lcl::ldouble(radius) : lcl::nilValue();
}

BUILTIN("getpoint")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString prompt = "";
    double x, y, z= 0.0;
    bool ref = false;

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ref = true;
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    RS_Vector result = RS_SCRIPTINGAPI->getPoint(Lisp_CommandEdit, qUtf8Printable(prompt), ref ? RS_Vector(x, y, z) : RS_Vector());

    if (result.valid)
    {
        lclValueVec *pt = new lclValueVec(3);
                    pt->at(0) = lcl::ldouble(result.x);
                    pt->at(1) = lcl::ldouble(result.y);
                    pt->at(2) = lcl::ldouble(result.z);

        return lcl::list(pt);
    }

    return lcl::nilValue();
}

BUILTIN("getreal")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    double result;
    QString prompt = "";

    if (args == 1)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    return RS_SCRIPTINGAPI->getReal(Lisp_CommandEdit,
                                    qUtf8Printable(prompt),
                                    result) ? lcl::ldouble(result) : lcl::nilValue();
}

BUILTIN("getstring")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    String result;
    QString prompt = "";
    bool cr = false;

    if (args == 2)
    {
        if(TRUE_PTR && T_PTR)
        {
            cr = true;
        }
        argsBegin++;
    }
    if (args >= 1)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    return RS_SCRIPTINGAPI->getString(Lisp_CommandEdit,
                                      cr,
                                      qUtf8Printable(prompt),
                                      result) ? lcl::string(result) : lcl::nilValue();

}

BUILTIN("getvar") {
    CHECK_ARGS_IS(1);

    ARG(lclString, var);

    return getvar(var->value());
}

BUILTIN("grdraw") {
    int args = CHECK_ARGS_BETWEEN(3, 4);

    ARG(lclSequence, start);
    ARG(lclSequence, end);
    ARG(lclInteger, color);
    bool highlight = false;

    RS_Vector startPnt, endPnt;

    if (args == 4)
    {
       ARG(lclInteger, hlight);
       if (hlight->value())
       {
           highlight = true;
       }
    }

    if (start->count() < 2)
    {
        return lcl::nilValue();
    }

    if (start->item(0)->type() == LCLTYPE::REAL)
    {
        const lclDouble *X = VALUE_CAST(lclDouble, start->item(0));
        startPnt.x = X->value();
    }
    if (start->item(0)->type() == LCLTYPE::INT)
    {
        const lclInteger *X = VALUE_CAST(lclInteger, start->item(0));
        startPnt.x = double(X->value());
    }
    if (start->item(1)->type() == LCLTYPE::REAL)
    {
        const lclDouble *Y = VALUE_CAST(lclDouble, start->item(1));
        startPnt.y = Y->value();
    }
    if (start->item(1)->type() == LCLTYPE::INT)
    {
        const lclInteger *Y = VALUE_CAST(lclInteger, start->item(1));
        startPnt.y = double(Y->value());
    }

    if (start->count() == 3)
    {
        if (start->item(2)->type() == LCLTYPE::REAL)
        {
            const lclDouble *Z = VALUE_CAST(lclDouble, start->item(2));
            startPnt.z = Z->value();
        }
        if (start->item(2)->type() == LCLTYPE::INT)
        {
            const lclInteger *Z = VALUE_CAST(lclInteger, start->item(2));
            startPnt.z = double(Z->value());
        }
    }

    if (end->count() < 2)
    {
        return lcl::nilValue();
    }

    if (end->item(0)->type() == LCLTYPE::REAL)
    {
        const lclDouble *X = VALUE_CAST(lclDouble, end->item(0));
        endPnt.x = X->value();
    }
    if (end->item(0)->type() == LCLTYPE::INT)
    {
        const lclInteger *X = VALUE_CAST(lclInteger, end->item(0));
        endPnt.x = double(X->value());
    }
    if (end->item(1)->type() == LCLTYPE::REAL)
    {
        const lclDouble *Y = VALUE_CAST(lclDouble, end->item(1));
        endPnt.y = Y->value();
    }
    if (end->item(1)->type() == LCLTYPE::INT)
    {
        const lclInteger *Y = VALUE_CAST(lclInteger, end->item(1));
        endPnt.y = double(Y->value());
    }

    if (end->count() == 3)
    {
        if (end->item(2)->type() == LCLTYPE::REAL)
        {
            const lclDouble *Z = VALUE_CAST(lclDouble, end->item(2));
            endPnt.z = Z->value();
        }
        if (end->item(2)->type() == LCLTYPE::INT)
        {
            const lclInteger *Z = VALUE_CAST(lclInteger, end->item(2));
            endPnt.z = double(Z->value());
        }
    }

    RS_SCRIPTINGAPI->grdraw(startPnt, endPnt, color->value(), highlight);

    return lcl::nilValue();
}

BUILTIN("grvecs")
{
    int args = CHECK_ARGS_BETWEEN(1, 2);

    ARG(lclSequence, vlist);
    if (args == 2)
    {
       ARG(lclSequence, trans);
       Q_UNUSED(trans)
    }

    if(vlist->count() < 2)
    {
        lcl::nilValue();
    }

    std::vector<grdraw_line_t> lines(0);

    for (int i = 0; i < vlist->count(); i++)
    {
        grdraw_line_t line;
        if (vlist->item(i)->type() == LCLTYPE::INT)
        {
            const lclInteger *c = VALUE_CAST(lclInteger, vlist->item(i));
            line.color = c->value();
            i++;
        }

        const lclList *start = VALUE_CAST(lclList, vlist->item(i++));
        const lclList *end = VALUE_CAST(lclList, vlist->item(i));

        if (start->count() < 2)
        {
            return lcl::nilValue();
        }

        if (start->item(0)->type() == LCLTYPE::REAL)
        {
            const lclDouble *X = VALUE_CAST(lclDouble, start->item(0));
            line.start.x = X->value();
        }
        if (start->item(0)->type() == LCLTYPE::INT)
        {
            const lclInteger *X = VALUE_CAST(lclInteger, start->item(0));
            line.start.x = double(X->value());
        }
        if (start->item(1)->type() == LCLTYPE::REAL)
        {
            const lclDouble *Y = VALUE_CAST(lclDouble, start->item(1));
            line.start.y = Y->value();
        }
        if (start->item(1)->type() == LCLTYPE::INT)
        {
            const lclInteger *Y = VALUE_CAST(lclInteger, start->item(1));
            line.start.y = double(Y->value());
        }

        if (start->count() == 3)
        {
            if (start->item(2)->type() == LCLTYPE::REAL)
            {
                const lclDouble *Z = VALUE_CAST(lclDouble, start->item(2));
                line.start.z = Z->value();
            }
            if (start->item(2)->type() == LCLTYPE::INT)
            {
                const lclInteger *Z = VALUE_CAST(lclInteger, start->item(2));
                line.start.z = double(Z->value());
            }
        }

        if (end->count() < 2)
        {
            return lcl::nilValue();
        }

        if (end->item(0)->type() == LCLTYPE::REAL)
        {
            const lclDouble *X = VALUE_CAST(lclDouble, end->item(0));
            line.end.x = X->value();
        }
        if (end->item(0)->type() == LCLTYPE::INT)
        {
            const lclInteger *X = VALUE_CAST(lclInteger, end->item(0));
            line.end.x = double(X->value());
        }
        if (end->item(1)->type() == LCLTYPE::REAL)
        {
            const lclDouble *Y = VALUE_CAST(lclDouble, end->item(1));
            line.end.y = Y->value();
        }
        if (end->item(1)->type() == LCLTYPE::INT)
        {
            const lclInteger *Y = VALUE_CAST(lclInteger, end->item(1));
            line.end.y = double(Y->value());
        }

        if (end->count() == 3)
        {
            if (end->item(2)->type() == LCLTYPE::REAL)
            {
                const lclDouble *Z = VALUE_CAST(lclDouble, end->item(2));
                line.end.z = Z->value();
            }
            if (end->item(2)->type() == LCLTYPE::INT)
            {
                const lclInteger *Z = VALUE_CAST(lclInteger, end->item(2));
                line.end.z = double(Z->value());
            }
        }

        lines.push_back(line);
    }

    if (lines.size())
    {
        RS_SCRIPTINGAPI->grvecs(lines);
    }

    return lcl::nilValue();
}

BUILTIN("hash-map")
{
    Q_UNUSED(name);
    return lcl::hash(argsBegin, argsEnd, true);
}

BUILTIN("help")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);

    if (args == 1)
    {
        ARG(lclString, str);
        RS_SCRIPTINGAPI->help(str->value().c_str());
        return lcl::nilValue();
    }

    RS_SCRIPTINGAPI->help();
    return lcl::nilValue();
}

BUILTIN("initget") {
    int args = CHECK_ARGS_BETWEEN(1, 2);
    int bit = 0;

    if (args == 2)
    {
        AG_INT(b);
        bit = b->value();
    }
    ARG(lclString, str);
    RS_SCRIPTINGAPI->initGet(bit, str->value().c_str());

    return lcl::nilValue();
}

BUILTIN("keys")
{
    CHECK_ARGS_IS(1);
    ARG(lclHash, hash);
    return hash->keys();
}

BUILTIN("keyword")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;
    if (lclKeyword* s = DYNAMIC_CAST(lclKeyword, arg))
      return s;
    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
      return lcl::keyword(":" + s->value());
    LCL_FAIL("keyword expects a keyword or string");
    return lcl::nilValue();
}

BUILTIN("last")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    LCL_CHECK(0 < seq->count(), "Index out of range");
    return seq->item(seq->count()-1);
}

BUILTIN("license")
{
    CHECK_ARGS_IS(0);
    QFile f(":/gpl-2.0.txt");
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        return lcl::nilValue();
    }
    QTextStream in(&f);
    std::cout << in.readAll().toStdString();

    return lcl::nilValue();
}

BUILTIN("list")
{
    Q_UNUSED(name);
    return lcl::list(argsBegin, argsEnd);
}

BUILTIN("listp")
{
    CHECK_ARGS_IS(1);
    return (DYNAMIC_CAST(lclList, *argsBegin)) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("load_dialog") {
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    return lcl::integer(RS_SCRIPTINGAPI->loadDialog(path->value().c_str()));
}

BUILTIN("log")
{
    BUILTIN_FUNCTION(log);
}

BUILTIN("logand")
{
    int argCount = CHECK_ARGS_AT_LEAST(0);
    int result = 0;
    [[maybe_unused]] double floatValue = 0;
    [[maybe_unused]] int intValue = 0;

    if (argCount == 0) {
        return lcl::integer(0);
    }
    else {
        CHECK_IS_NUMBER(argsBegin->ptr());
        if (INT_PTR) {
            ADD_INT_VAL(*intVal);
            intValue = intVal->value();
            if (argCount == 1) {
                return lcl::integer(intValue);
            }
            else {
                result = intValue;
            }
        }
        else {
            ADD_FLOAT_VAL(*floatVal);
            floatValue = floatVal->value();
            if (argCount == 1) {
                return lcl::integer(int(floatValue));
            }
            else {
                result = int(floatValue);
            }
        }
    }
    for (auto it = argsBegin; it != argsEnd; it++) {
        CHECK_IS_NUMBER(it->ptr());
        if (it->ptr()->type() == LCLTYPE::INT) {
            const lclInteger* i = VALUE_CAST(lclInteger, *it);
            result = result & i->value();
        }
        else {
            const lclDouble* i = VALUE_CAST(lclDouble, *it);
            result = result & int(i->value());
        }
    }
    return lcl::integer(result);
}

BUILTIN("log10")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR) {
        ADD_FLOAT_VAL(*lhs)
        if (lhs->value() < 0) {
            return lcl::nilValue();
        }
        return lcl::ldouble(log10(lhs->value())); }
    else {
        ADD_INT_VAL(*lhs)
        if (lhs->value() < 0) {
            return lcl::nilValue();
        }
        return lcl::ldouble(log10(lhs->value())); }
}

BUILTIN("macro?")
{
    CHECK_ARGS_IS(1);

    // Macros are implemented as lambdas, with a special flag.
    const lclLambda* lambda = DYNAMIC_CAST(lclLambda, *argsBegin);
    return lcl::boolean((lambda != NULL) && lambda->isMacro());
}

BUILTIN("map")
{
    CHECK_ARGS_IS(2);
    lclValuePtr op = *argsBegin++; // this gets checked in APPLY
    ARG(lclSequence, source);

    const int length = source->count();
    lclValueVec* items = new lclValueVec(length);
    auto it = source->begin();
    for (int i = 0; i < length; i++) {
      items->at(i) = APPLY(op, it+i, it+i+1);
    }

    return  lcl::list(items);
}

BUILTIN("mapcar")
{
    int argCount = CHECK_ARGS_AT_LEAST(2);
    int i = 0, count = 0, offset = 0;
    int listCount = argCount-1;

    std::vector<int> listCounts(static_cast<int>(listCount));
    const lclValuePtr op = EVAL(argsBegin++->ptr(), NULL);

    for (auto it = argsBegin++; it != argsEnd; it++) {
        const lclSequence* seq = VALUE_CAST(lclSequence, *it);
        const int length = seq->count();
        listCounts[i++] = length;
        offset += length;
        if (count < length) {
            count = length;
        }
    }

    std::vector<int> newListCounts(count);
    std::vector<lclValueVec *> valItems(count);
    lclValueVec* items = new lclValueVec(offset);
    lclValueVec* result = new lclValueVec(count);

    offset = 0;
    for (auto it = --argsBegin; it != argsEnd; ++it) {
        const lclSequence* seq = STATIC_CAST(lclSequence, *it);
        const int length = seq->count();
        std::copy(seq->begin(), seq->end(), items->begin() + offset);
        offset += length;
    }

    for (auto l = 0; l < count; l++) {
        newListCounts[l] = 0;
        valItems[l] = { new lclValueVec(listCount+1) };
        valItems[l]->at(0) = op;
    }

    offset = 0;
    for (auto n = 0; n < listCount; n++) {
        for (auto l = 0; l < count; l++) {
            if (listCounts[n] > l) {
                valItems[l]->at(n + 1) = items->at(offset + l);
                newListCounts[l] += 1;
            }
        }
        offset += listCounts[n];
    }

    for (auto l = 0; l < count; l++) {
        for (auto v = listCount - newListCounts[l]; v > 0; v--) {
            valItems[l]->erase(std::next(valItems[l]->begin()));
        }
        lclList* List = new lclList(valItems[l]);
        result->at(l) = EVAL(List, NULL);
    }
    return lcl::list(result);
}


BUILTIN("max")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    bool hasFloat = ARGS_HAS_FLOAT;
    bool unset = true;
    [[maybe_unused]] double floatValue = 0;
    [[maybe_unused]] int intValue = 0;

    if (count == 1)
    {
        if (hasFloat) {
            ADD_FLOAT_VAL(*floatVal);
            floatValue = floatVal->value();
            return lcl::ldouble(floatValue);
        }
        else {
            ADD_INT_VAL(*intVal);
            intValue = intVal->value();
            return lcl::integer(intValue);
        }
    }

    if (hasFloat) {
        do {
            if (FLOAT_PTR) {
                if (unset) {
                    unset = false;
                    ADD_FLOAT_VAL(*floatVal);
                    floatValue = floatVal->value();
                }
                else {
                    ADD_FLOAT_VAL(*floatVal)
                    if (floatVal->value() > floatValue) {
                        floatValue = floatVal->value();
                    }
                }
            }
            else {
                if (unset) {
                    unset = false;
                    ADD_INT_VAL(*intVal);
                    floatValue = double(intVal->value());
                }
                else {
                    ADD_INT_VAL(*intVal);
                    if (intVal->value() > floatValue)
                    {
                        floatValue = double(intVal->value());
                    }
                }
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::ldouble(floatValue);
    } else {
        ADD_INT_VAL(*intVal);
        intValue = intVal->value();
        argsBegin++;
        do {
            ADD_INT_VAL(*intVal);
            if (intVal->value() > intValue)
            {
                intValue = intVal->value();
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::integer(intValue);
    }
}

BUILTIN("member?")
{
    CHECK_ARGS_IS(2);
    lclValuePtr op = *argsBegin++;
    ARG(lclSequence, seq);

    const int length = seq->count();
    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());

    for (int i = 0; i < length; i++) {
        if (items->at(i)->print(true) == op->print(true)) {
            return lcl::trueValue();
        }
    }
    return lcl::nilValue();
}

BUILTIN("meta")
{
    CHECK_ARGS_IS(1);
    lclValuePtr obj = *argsBegin++;

    return obj->meta();
}

BUILTIN("min")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    bool hasFloat = ARGS_HAS_FLOAT;
    bool unset = true;
    [[maybe_unused]] double floatValue = 0;
    [[maybe_unused]] int intValue = 0;

    if (count == 1)
    {
        if (hasFloat) {
            ADD_FLOAT_VAL(*floatVal);
            floatValue = floatVal->value();
            return lcl::ldouble(floatValue);
        }
        else {
            ADD_INT_VAL(*intVal);
            intValue = intVal->value();
            return lcl::integer(intValue);
        }
    }

    if (hasFloat) {
        do {
            if (FLOAT_PTR) {
                if (unset) {
                    unset = false;
                    ADD_FLOAT_VAL(*floatVal);
                    floatValue = floatVal->value();
                }
                else {
                    ADD_FLOAT_VAL(*floatVal)
                    if (floatVal->value() < floatValue) {
                        floatValue = floatVal->value();
                    }
                }
            }
            else {
                if (unset) {
                    unset = false;
                    ADD_INT_VAL(*intVal);
                    floatValue = double(intVal->value());
                }
                else {
                    ADD_INT_VAL(*intVal);
                    if (intVal->value() < floatValue) {
                        floatValue = double(intVal->value());
                    }
                }
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::ldouble(floatValue);
    } else {
        ADD_INT_VAL(*intVal);
        intValue = intVal->value();
        argsBegin++;
        do {
            ADD_INT_VAL(*intVal);
            if (intVal->value() < intValue) {
                intValue = intVal->value();
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::integer(intValue);
    }
}

BUILTIN("minus?")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() < 0.0);

    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() < 0);
    }
    return lcl::falseValue();
}

BUILTIN("minusp")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return val->value() < 0 ? lcl::trueValue() : lcl::nilValue();
    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return val->value() < 0 ? lcl::trueValue() : lcl::nilValue();
    }
    return lcl::nilValue();
}

BUILTIN("mode_tile")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, key);
    AG_INT(mode);

    return RS_SCRIPTINGAPI->modeTile(key->value().c_str(),
                                     mode->value()) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("new_dialog")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, dlgName);
    AG_INT(id);

    return RS_SCRIPTINGAPI->newDialog(dlgName->value().c_str(),
                                      id->value()) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("nth")
{
    // twisted parameter for both LISPs!
    CHECK_ARGS_IS(2);
    int n;

    if(INT_PTR)
    {
        AG_INT(index);
        ARG(lclSequence, seq);
        n = index->value();
        LCL_CHECK(n >= 0 && n < seq->count(), "Index out of range");
        return seq->item(n);
    }
    else if(FLOAT_PTR) {
        // add dummy for error msg
        AG_INT(index);
        [[maybe_unused]] const String dummy = index->print(true);
        return lcl::nilValue();
    }
    else {
        ARG(lclSequence, seq);
        AG_INT(index);
        n = index->value();
        LCL_CHECK(n >= 0 && n < seq->count(), "Index out of range");
        return seq->item(n);
    }
}

BUILTIN("null")
{
    CHECK_ARGS_IS(1);
    if (NIL_PTR) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("number?")
{
    CHECK_ARGS_IS(1);
    return lcl::boolean(DYNAMIC_CAST(lclInteger, *argsBegin) ||
            DYNAMIC_CAST(lclDouble, *argsBegin));
}

BUILTIN("numberp")
{
    CHECK_ARGS_IS(1);
    return (DYNAMIC_CAST(lclInteger, *argsBegin) ||
            DYNAMIC_CAST(lclDouble, *argsBegin)) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("open")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, filename);
    ARG(lclString, m);
    const char mode = std::tolower(m->value().c_str()[0]);
    lclFile* pf = new lclFile(filename->value().c_str(), mode);

    return pf->open();
}

BUILTIN("pix_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x);
    AG_INT(y);
    AG_INT(width);
    AG_INT(height);
    ARG(lclString, path);

    return RS_SCRIPTINGAPI->pixImage(x->value(),
                                     y->value(),
                                     width->value(),
                                     height->value(),
                                     path->value().c_str()) ? lcl::string(path->value()) : lcl::nilValue();
}

BUILTIN("polar")
{
    CHECK_ARGS_IS(3);
    double angle, dist, x, y, z;
    ARG(lclSequence, seq);

    CHECK_IS_NUMBER(seq->item(0))
    CHECK_IS_NUMBER(seq->item(1))

    if (FLOAT_PTR) {
        ADD_FLOAT_VAL(*floatAngle)
        angle = floatAngle->value();
    }
    else
    {
        ADD_INT_VAL(*intAngle)
        angle = double(intAngle->value());
    }
    argsBegin++;
    if (FLOAT_PTR) {
        ADD_FLOAT_VAL(*floatDist)
        dist = floatDist->value();
    }
    else
    {
        ADD_INT_VAL(*intDist)
        dist = double(intDist->value());
    }

    if(seq->count() == 2)
    {
        if (seq->item(0)->type() == LCLTYPE::INT)
        {
            const lclInteger* intX = VALUE_CAST(lclInteger, seq->item(0));
            x = double(intX->value());
        }
        else
        {
            const lclDouble* floatX = VALUE_CAST(lclDouble, seq->item(0));
            x = floatX->value();
        }

        if (seq->item(1)->type() == LCLTYPE::INT)
        {
            const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(1));
            y = double(intY->value());
        }
        else
        {
            const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(1));
            y = floatY->value();
        }

        lclValueVec* items = new lclValueVec(2);
        items->at(0) = lcl::ldouble(x + dist * std::sin(angle));
        items->at(1) = lcl::ldouble(y + dist * std::cos(angle));
        return lcl::list(items);
    }

    if(seq->count() == 3)
    {
        CHECK_IS_NUMBER(seq->item(2))

        if (seq->item(0)->type() == LCLTYPE::INT)
        {
            const lclInteger* intX = VALUE_CAST(lclInteger, seq->item(0));
            x = double(intX->value());
        }
        else
        {
            const lclDouble* floatX = VALUE_CAST(lclDouble, seq->item(0));
            x = floatX->value();
        }

        if (seq->item(1)->type() == LCLTYPE::INT)
        {
            const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(1));
            y = double(intY->value());
        }
        else
        {
            const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(1));
            y = floatY->value();
        }

        if (seq->item(2)->type() == LCLTYPE::INT)
        {
            const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(2));
            z = double(intY->value());
        }
        else
        {
            const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(2));
            z = floatY->value();
        }
        lclValueVec* items = new lclValueVec(3);
        items->at(0) = lcl::ldouble(x + dist * std::sin(angle));
        items->at(1) = lcl::ldouble(y + dist * std::cos(angle));
        items->at(2) = lcl::ldouble(z);
        return lcl::list(items);
    }
    return lcl::nilValue();
}

BUILTIN("pr-str")
{
    Q_UNUSED(name);
    return lcl::string(printValues(argsBegin, argsEnd, " ", true));
}

BUILTIN("prin1")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    if (args == 0) {
        std::cout << std::endl;
        return lcl::nullValue();
    }
    lclFile* pf = NULL;
    LCLTYPE type = argsBegin->ptr()->type();
    String boolean = argsBegin->ptr()->print(true);
    lclValueIter value = argsBegin;

    if (args == 2) {
        argsBegin++;
        if (argsBegin->ptr()->print(true) != "nil") {
            pf = VALUE_CAST(lclFile, *argsBegin);
        }
    }
    if (boolean == "nil") {
        if (pf) {
            pf->writeLine("\"nil\"");
        }
        else {
            std::cout << "\"nil\"";
        }
            return lcl::nilValue();
    }
    if (boolean == "false") {
        if (pf) {
            pf->writeLine("\"false\"");
        }
        else {
            std::cout << "\"false\"";
        }
            return lcl::falseValue();
    }
    if (boolean == "true") {
        if (pf) {
            pf->writeLine("\"true\"");
        }
        else {
            std::cout << "\"true\"";
        }
            return lcl::trueValue();
    }
    if (boolean == "T") {
        if (pf) {
            pf->writeLine("\"T\"");
        }
        else {
            std::cout << "\"T\"";
        }
            return lcl::trueValue();
    }

    switch(type) {
        case LCLTYPE::FILE: {
            lclFile* f = VALUE_CAST(lclFile, *value);
            char filePtr[32];
            //sprintf(filePtr, "%p", f->value());
            snprintf(filePtr, 32, "%p", f->value());
            const String file = filePtr;
            if (pf) {
                pf->writeLine("\"" + file + "\"");
            }
            else {
                std::cout << "\"" << file << "\"";
            }
            return f;
        }
        case LCLTYPE::INT: {
            lclInteger* i = VALUE_CAST(lclInteger, *value);
            if (pf) {
                pf->writeLine("\"" + i->print(true) + "\"");
            }
            else {
                std::cout << "\"" << i->print(true) << "\"";
            }
            return i;
        }
        case LCLTYPE::LIST: {
            lclList* list = VALUE_CAST(lclList, *value);
            if (pf) {
                pf->writeLine("\"" + list->print(true) + "\"");
            }
            else {
                std::cout << "\"" << list->print(true) << "\"";
            }
            return list;
        }
        case LCLTYPE::MAP: {
            lclHash* hash = VALUE_CAST(lclHash, *value);
            if (pf) {
                pf->writeLine("\"" + hash->print(true) + "\"");
            }
            else {
                std::cout << "\"" << hash->print(true) << "\"";
            }
            return hash;
         }
        case LCLTYPE::REAL: {
            lclDouble* d = VALUE_CAST(lclDouble, *value);
            if (pf) {
                pf->writeLine("\"" + d->print(true) + "\"");
            }
            else {
                std::cout << "\"" << d->print(true) << "\"";
            }
            return d;
        }
        case LCLTYPE::STR: {
            lclString* str = VALUE_CAST(lclString, *value);
            if (pf) {
                pf->writeLine("\"" + str->value() + "\"");
            }
            else {
                std::cout << "\"" << str->value() << "\"";
            }
            return str;
        }
        case LCLTYPE::SYM: {
            lclSymbol* sym = VALUE_CAST(lclSymbol, *value);
            if (pf) {
                pf->writeLine("\"" + sym->value() + "\"");
            }
            else {
                std::cout << "\"" << sym->value() << "\"";
            }
            return sym;
        }
        case LCLTYPE::VEC: {
            lclVector* vector = VALUE_CAST(lclVector, *value);
            if (pf) {
                pf->writeLine("\"" + vector->print(true) + "\"");
            }
            else {
                std::cout << "\"" << vector->print(true) << "\"";
            }
            return vector;
        }
        case LCLTYPE::KEYW: {
            lclKeyword* keyword = VALUE_CAST(lclKeyword, *value);
            if (pf) {
                pf->writeLine("\"" + keyword->print(true) + "\"");
            }
            else {
                std::cout << "\"" << keyword->print(true) << "\"";
            }
            return keyword;
        }
        default: {
            if (pf) {
                pf->writeLine("\"nil\"");
            }
            else {
                std::cout << "\"nil\"";
            }
            return lcl::nilValue();
        }
    }

    if (pf) {
        pf->writeLine("\"nil\"");
    }
    else {
        std::cout << "\"nil\"";
    }
        return lcl::nilValue();
}

BUILTIN("princ")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    if (args == 0) {
        std::cout << std::endl;
        fflush(stdout);
        return lcl::nullValue();
    }
    lclFile* pf = NULL;
    LCLTYPE type = argsBegin->ptr()->type();
    String boolean = argsBegin->ptr()->print(true);
    lclValueIter value = argsBegin;

    if (args == 2) {
        argsBegin++;
        if (argsBegin->ptr()->print(true) != "nil") {
            pf = VALUE_CAST(lclFile, *argsBegin);
        }
    }
    if (boolean == "nil") {
        if (pf) {
            pf->writeLine("nil");
        }
        else {
            std::cout << "nil";
        }
            return lcl::nilValue();
    }
    if (boolean == "false") {
        if (pf) {
            pf->writeLine("false");
        }
        else {
            std::cout << "false";
        }
            return lcl::falseValue();
    }
    if (boolean == "true") {
        if (pf) {
            pf->writeLine("true");
        }
        else {
            std::cout << "true";
        }
            return lcl::trueValue();
    }
    if (boolean == "T") {
        if (pf) {
            pf->writeLine("T");
        }
        else {
            std::cout << "T";
        }
            return lcl::trueValue();
    }

    switch(type) {
        case LCLTYPE::FILE: {
            lclFile* f = VALUE_CAST(lclFile, *value);
            char filePtr[32];
            //sprintf(filePtr, "%p", f->value());
            snprintf(filePtr, 32, "%p", f->value());
            const String file = filePtr;
            if (pf) {
                pf->writeLine(file);
            }
            else {
                std::cout << file;
            }
            return f;
        }
        case LCLTYPE::INT: {
            lclInteger* i = VALUE_CAST(lclInteger, *value);
            if (pf) {
                pf->writeLine(i->print(true));
            }
            else {
                std::cout << i->print(true);
            }
            return i;
        }
        case LCLTYPE::LIST: {
            lclList* list = VALUE_CAST(lclList, *value);
            if (pf) {
                pf->writeLine(list->print(true));
            }
            else {
                std::cout << list->print(true);
            }
            return list;
        }
        case LCLTYPE::MAP: {
            lclHash* hash = VALUE_CAST(lclHash, *value);
            if (pf) {
                pf->writeLine(hash->print(true));
            }
            else {
                std::cout << hash->print(true);
            }
            return hash;
         }
        case LCLTYPE::REAL: {
            lclDouble* d = VALUE_CAST(lclDouble, *value);
            if (pf) {
                pf->writeLine(d->print(true));
            }
            else {
                std::cout << d->print(true);
            }
            return d;
        }
        case LCLTYPE::STR: {
            lclString* str = VALUE_CAST(lclString, *value);
            if (pf) {
                pf->writeLine(str->value());
            }
            else {
                std::cout << str->value();
                fflush(stdout);
            }
            return str;
        }
        case LCLTYPE::SYM: {
            lclSymbol* sym = VALUE_CAST(lclSymbol, *value);
            if (pf) {
                pf->writeLine(sym->value());
            }
            else {
                std::cout << sym->value();
            }
            return sym;
        }
        case LCLTYPE::VEC: {
            lclVector* vector = VALUE_CAST(lclVector, *value);
            if (pf) {
                pf->writeLine(vector->print(true));
            }
            else {
                std::cout << vector->print(true);
            }
            return vector;
        }
        case LCLTYPE::KEYW: {
            lclKeyword* keyword = VALUE_CAST(lclKeyword, *value);
            if (pf) {
                pf->writeLine(keyword->print(true));
            }
            else {
                std::cout << keyword->print(true);
            }
            return keyword;
        }
        default: {
            if (pf) {
                pf->writeLine("nil");
            }
            else {
                std::cout << "nil";
            }
            return lcl::nilValue();
        }
        fflush(stdout);
    }

    if (pf) {
        pf->writeLine("nil");
    }
    else {
        std::cout << "nil";
    }
        return lcl::nilValue();
}

BUILTIN("print")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    if (args == 0) {
        std::cout << std::endl;
        return lcl::nullValue();
    }
    lclFile* pf = NULL;
    LCLTYPE type = argsBegin->ptr()->type();
    String boolean = argsBegin->ptr()->print(true);
    lclValueIter value = argsBegin;

    if (args == 2) {
        argsBegin++;
        if (argsBegin->ptr()->print(true) != "nil") {
            pf = VALUE_CAST(lclFile, *argsBegin);
        }
    }
    if (boolean == "nil") {
        if (pf) {
            pf->writeLine("\n\"nil\" ");
        }
        else {
            std::cout << "\n\"nil\" ";
        }
            return lcl::nilValue();
    }
    if (boolean == "false") {
        if (pf) {
            pf->writeLine("\n\"false\" ");
        }
        else {
            std::cout << "\n\"false\" ";
        }
            return lcl::falseValue();
    }
    if (boolean == "true") {
        if (pf) {
            pf->writeLine("\n\"true\" ");
        }
        else {
            std::cout << "\n\"true\" ";
        }
            return lcl::trueValue();
    }
    if (boolean == "T") {
        if (pf) {
            pf->writeLine("\n\"T\" ");
        }
        else {
            std::cout << "\n\"T\" ";
        }
            return lcl::trueValue();
    }

    switch(type) {
        case LCLTYPE::FILE: {
            lclFile* f = VALUE_CAST(lclFile, *value);
            char filePtr[32];
            //sprintf(filePtr, "%p", f->value());
            snprintf(filePtr, 32, "%p", f->value());
            const String file = filePtr;
            if (pf) {
                pf->writeLine("\n\"" + file + "\" ");
            }
            else {
                std::cout << "\n\"" << file << "\" ";
            }
            return f;
        }
        case LCLTYPE::INT: {
            lclInteger* i = VALUE_CAST(lclInteger, *value);
            if (pf) {
                pf->writeLine("\n\"" + i->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << i->print(true) << "\" ";
            }
            return i;
        }
        case LCLTYPE::LIST: {
            lclList* list = VALUE_CAST(lclList, *value);
            if (pf) {
                pf->writeLine("\n\"" + list->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << list->print(true) << "\" ";
            }
            return list;
        }
        case LCLTYPE::MAP: {
            lclHash* hash = VALUE_CAST(lclHash, *value);
            if (pf) {
                pf->writeLine("\n\"" + hash->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << hash->print(true) << "\" ";
            }
            return hash;
         }
        case LCLTYPE::REAL: {
            lclDouble* d = VALUE_CAST(lclDouble, *value);
            if (pf) {
                pf->writeLine("\n\"" + d->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << d->print(true) << "\" ";
            }
            return d;
        }
        case LCLTYPE::STR: {
            lclString* str = VALUE_CAST(lclString, *value);
            if (pf) {
                pf->writeLine("\n\"" + str->value() + "\" ");
            }
            else {
                std::cout << "\n\"" << str->value() << "\" ";
            }
            return str;
        }
        case LCLTYPE::SYM: {
            lclSymbol* sym = VALUE_CAST(lclSymbol, *value);
            if (pf) {
                pf->writeLine("\n\"" + sym->value() + "\" ");
            }
            else {
                std::cout << "\n\"" << sym->value() << "\" ";
            }
            return sym;
        }
        case LCLTYPE::VEC: {
            lclVector* vector = VALUE_CAST(lclVector, *value);
            if (pf) {
                pf->writeLine("\n\"" + vector->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << vector->print(true) << "\" ";
            }
            return vector;
        }
        case LCLTYPE::KEYW: {
            lclKeyword* keyword = VALUE_CAST(lclKeyword, *value);
            if (pf) {
                pf->writeLine("\n\"" + keyword->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << keyword->print(true) << "\" ";
            }
            return keyword;
        }
        default: {
            if (pf) {
                pf->writeLine("\n\"nil\" ");
            }
            else {
                std::cout << "\n\"nil\" ";
            }
            return lcl::nilValue();
        }
    }

    if (pf) {
        pf->writeLine("\n\"nil\" ");
    }
    else {
        std::cout << "\n\"nil\" ";
    }
        return lcl::nilValue();
}

BUILTIN("println")
{
    Q_UNUSED(name);
    std::cout << printValues(argsBegin, argsEnd, " ", false) << std::endl;
    return lcl::nilValue();
}

BUILTIN("prn")
{
    Q_UNUSED(name);
    std::cout << printValues(argsBegin, argsEnd, " ", true) << std::endl;
    return lcl::nilValue();
}

BUILTIN("prompt")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    RS_SCRIPTINGAPI->prompt(Lisp_CommandEdit, str->value().c_str());

    return lcl::nilValue();
}

BUILTIN("py-eval-float")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    double result;
    QString error;
    int err = RS_PYTHON->evalFloat(com->value().c_str(), result, error);
    if (err == 0)
    {
        return lcl::ldouble(result);
    }
    std::cout << error.toStdString() << std::endl;
    LCL_FAIL("'py-eval-float' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-eval-integer")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    int result;
    QString error;
    int err = RS_PYTHON->evalInteger(com->value().c_str(), result, error);
    if (err == 0)
    {
        return lcl::integer(result);
    }
    std::cout << error.toStdString() << std::endl;
    LCL_FAIL("'py-eval-integer' exec python failed");
     return lcl::nilValue();
}

BUILTIN("py-eval-string")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    QString result;
    QString error;
    int err = RS_PYTHON->evalString(com->value().c_str(), result, error);
    if (err == 0)
    {
        return lcl::string(result.toStdString());
    }
    std::cout << error.toStdString() << std::endl;
    LCL_FAIL("'py-eval-string' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-eval-value")
{
    CHECK_ARGS_IS(1);
    bool is_map = false;
    ARG(lclString, com);
    QString result;
    QString error;
    std::string command = "print(";
    command += com->value();
    command += ")";
    int err = RS_PYTHON->evalString(command.c_str(), result, error);

    if (err == 0)
    {
        for (auto i = result.size()-1; i > 0; --i)
        {
            if (result.at(i) == ':')
            {
                is_map = true;
                result.remove(i-1, 2);
                continue;
            }

            if (is_map && result.at(i) == '\'')
            {
                is_map = false;
                result.replace(i, 1, ":");
            }
        }

        static const QRegularExpression dq = QRegularExpression(QStringLiteral("[\"]"));
        static const QRegularExpression q = QRegularExpression(QStringLiteral("[']"));
        static const QRegularExpression rb = QRegularExpression(QStringLiteral("[(]"));
        result.remove(',');
        result.replace(dq, "\\\"");
        result.replace(q, "\"");
        result.replace(rb, "'(");
        return EVAL(readStr(result.toStdString()), NULL);
    }
    std::cout << error.toStdString();
    LCL_FAIL("'py-eval-value' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-simple-string")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    return lcl::integer(RS_PYTHON->runString(com->value().c_str()));
}

BUILTIN("py-simple-file")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    return lcl::integer(RS_PYTHON->runFile(com->value().c_str()));
}

BUILTIN("rand")
{
    CHECK_ARGS_IS(0);
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);

    return lcl::ldouble(dis(gen));
}

BUILTIN("rand-int")
{
    CHECK_ARGS_IS(1);
    AG_INT(max);
    return lcl::integer(rand() % max->value());
}

BUILTIN("read-string")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    return readStr(str->value());
}

BUILTIN("read-line")
{
    if (!CHECK_ARGS_AT_LEAST(0))
    {
        QString s;
        bool fallback = true;
        if (fallback)
        {
            s = RS_SCRIPTINGAPI->getStrDlg("Enter a text:").c_str();

            return lcl::string(s.toStdString());
        }
        else
        {
            //line = Lisp_CommandEdit->getline(str->value().c_str());
            return lcl::string(s.toStdString());
        }
    }
    ARG(lclFile, pf);

    return pf->readLine();
}

BUILTIN("read-char")
{
    if (!CHECK_ARGS_AT_LEAST(0))
    {
        return lcl::integer(int(RS_SCRIPTINGAPI->readChar()));
    }
    ARG(lclFile, pf);

    return pf->readChar();
}
#if 0
BUILTIN("readline")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    return readline(str->value());
}
#endif
BUILTIN("rem")
{
    CHECK_ARGS_AT_LEAST(2);
    if (ARGS_HAS_FLOAT) {
        [[maybe_unused]] double floatValue = 0;
        if (FLOAT_PTR) {
            ADD_FLOAT_VAL(*floatVal)
            floatValue = floatValue + floatVal->value();
            LCL_CHECK(floatVal->value() != 0.0, "Division by zero");
    }
    else {
        ADD_INT_VAL(*intVal)
        floatValue = floatValue + double(intVal->value());
        LCL_CHECK(intVal->value() != 0, "Division by zero");
    }
    argsBegin++;
    do {
        if (FLOAT_PTR) {
            ADD_FLOAT_VAL(*floatVal)
            floatValue = fmod(floatValue, floatVal->value());
            LCL_CHECK(floatVal->value() != 0.0, "Division by zero");
        }
        else {
            ADD_INT_VAL(*intVal)
            floatValue = fmod(floatValue, double(intVal->value()));
            LCL_CHECK(intVal->value() != 0, "Division by zero");
        }
        argsBegin++;
    } while (argsBegin != argsEnd);
    return lcl::ldouble(floatValue);
    } else {
        [[maybe_unused]] int intValue = 0;
        ADD_INT_VAL(*intVal) // +
        intValue = intValue + intVal->value();
        LCL_CHECK(intVal->value() != 0, "Division by zero");
        argsBegin++;
        do {
            ADD_INT_VAL(*intVal)
            intValue = int(fmod(double(intValue), double(intVal->value())));
            LCL_CHECK(intVal->value() != 0, "Division by zero");
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::integer(intValue);
    }
}

BUILTIN("reset!")
{
    CHECK_ARGS_IS(2);
    ARG(lclAtom, atom);
    return atom->reset(*argsBegin);
}

BUILTIN("rest")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }
    ARG(lclSequence, seq);
    return seq->rest();
}

BUILTIN("reverse")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }
    ARG(lclSequence, seq);
    return seq->reverse(seq->begin(), seq->end());
}

BUILTIN("seq")
{
    CHECK_ARGS_IS(1);
    lclValuePtr arg = *argsBegin++;
    if (arg == lcl::nilValue()) {
        return lcl::nilValue();
    }
    if (const lclSequence* seq = DYNAMIC_CAST(lclSequence, arg)) {
        return seq->isEmpty() ? lcl::nilValue()
                              : lcl::list(seq->begin(), seq->end());
    }
    if (const lclString* strVal = DYNAMIC_CAST(lclString, arg)) {
        const String str = strVal->value();
        int length = str.length();
        if (length == 0)
            return lcl::nilValue();

        lclValueVec* items = new lclValueVec(length);
        for (int i = 0; i < length; i++) {
            (*items)[i] = lcl::string(str.substr(i, 1));
        }
        return lcl::list(items);
    }
    LCL_FAIL("'%s' is not a string or sequence", arg->print(true).c_str());
    return lcl::nilValue();
}

BUILTIN("set_tile")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, key);
    ARG(lclString, val);

    return RS_SCRIPTINGAPI->setTile(key->value().c_str(),
                                    val->value().c_str()) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("setvar")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, id);

    double v1=0.0, v2=0.0;
    std::string str_value = "";

    if(INT_PTR)
    {
        ARG(lclInteger, num);
        v1 = static_cast<double>(num->value());
    }

    else if (FLOAT_PTR)
    {
        ARG(lclDouble, num);
        v1 = num->value();
    }

    else if (STR_PTR)
    {
        ARG(lclString, str);
        str_value = str->value();
    }

    else if (LIST_PTR)
    {
        ARG(lclSequence, seq);

        if(seq->count() > 1)
        {
            if (seq->item(0)->type() == LCLTYPE::INT)
            {
                const lclInteger* intX = VALUE_CAST(lclInteger, seq->item(0));
                v1 = double(intX->value());
            }
            else
            {
                const lclDouble* floatX = VALUE_CAST(lclDouble, seq->item(0));
                v1 = floatX->value();
            }

            if (seq->item(1)->type() == LCLTYPE::INT)
            {
                const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(1));
                v2 = double(intY->value());
            }
            else
            {
                const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(1));
                v2 = floatY->value();
            }
        }
        else
        {
            LCL_FAIL("set SYSVAR failed!");
        }
    }

    else
    {
        LCL_FAIL("set SYSVAR failed!");
    }

    if(!RS_SCRIPTINGAPI->setvar(id->value().c_str(), v1, v2, str_value.c_str()))
    {
        LCL_FAIL("set SYSVAR failed!");
    }

    return getvar(id->value());
}

BUILTIN("sin")
{
    BUILTIN_FUNCTION(sin);
}

BUILTIN("slurp")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, filename);

    std::ios_base::openmode openmode =
        std::ios::ate | std::ios::in | std::ios::binary;
    std::ifstream file(filename->value().c_str(), openmode);
    LCL_CHECK(!file.fail(), "Cannot open %s", filename->value().c_str());

    String data;
    data.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    data.append(std::istreambuf_iterator<char>(file.rdbuf()),
                std::istreambuf_iterator<char>());

    return lcl::string(data);
}

BUILTIN("sqrt")
{
    BUILTIN_FUNCTION(sqrt);
}

BUILTIN("ssadd")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);

    unsigned int id = 0;
    unsigned int ss = 0;
    unsigned int newss;

    if (args == 2)
    {
        ARG(lclEname, en);
        ARG(lclSelectionSet, set);

        id = en->value();
        ss = set->value();
    }

    return RS_SCRIPTINGAPI->ssadd(id, ss, newss)
            ? lcl::selectionset(newss) : lcl::nilValue();
}

BUILTIN("ssdel")
{
    CHECK_ARGS_IS(2);
    ARG(lclEname, en);
    ARG(lclSelectionSet, set);

    return RS_SCRIPTINGAPI->ssdel(en->value(), set->value())
            ? lcl::selectionset(set->value()) : lcl::nilValue();
}

BUILTIN("ssget")
{
    int args = CHECK_ARGS_BETWEEN(0, 3);
    unsigned int id;

    if (args == 0)
    {
        if (RS_SCRIPTINGAPI->getSelection(id))
        {
            return lcl::selectionset(id);
        }
    }

    std::vector<unsigned int> selection_set;

    if (args > 0 && STR_PTR)
    {
        ARG(lclString, sel);
        const QString &selMethod = sel->value().c_str();

        if(selMethod.contains("X", Qt::CaseInsensitive))
        {
            if (args == 1 && !RS_SCRIPTINGAPI->selectAll(selection_set))
            {
                return lcl::nilValue();
            }

            else if (args == 2)
            {
                ARG(lclSequence, seq);
                const int length = seq->count();
                lclValueVec* items = new lclValueVec(length);
                std::copy(seq->begin(), seq->end(), items->begin());

                RS_ScriptingApiData apiData;

                if (!getApiData(items, apiData))
                {
                    return lcl::nilValue();
                }

                if (RS_SCRIPTINGAPI->getSelectionByData(apiData, selection_set))
                {
                    return lcl::selectionset(id);
                }
            }

            else
            {
                return lcl::nilValue();
            }
        }

        if (selMethod.contains(":S", Qt::CaseInsensitive))
        {
            qDebug() << "[getSingleSelection] :S";
            if (!RS_SCRIPTINGAPI->getSingleSelection(selection_set))
            {
                return lcl::nilValue();
            }
        }

        if (selMethod.contains("A", Qt::CaseInsensitive))
        {
            qDebug() << "[selectAll] A";

            if (!selMethod.contains("-A", Qt::CaseInsensitive) && !RS_SCRIPTINGAPI->selectAll(selection_set))
            {
                return lcl::nilValue();
            }
        }

        if (selMethod.contains("L", Qt::CaseInsensitive))
        {
            unsigned int id = RS_SCRIPTINGAPI->entlast();
            if(id > 0 && !selMethod.contains("-L", Qt::CaseInsensitive))
            {
                selection_set.push_back(id);
            }
        }

        if (selMethod.contains("P", Qt::CaseInsensitive))
        {
            unsigned int id = RS_SCRIPTINGAPI->entnext();
            if(id > 0 && !selMethod.contains("-P", Qt::CaseInsensitive))
            {
                selection_set.push_back(id);
            }
        }
    }

    unsigned int length = selection_set.size();

    if (length == 0)
    {
        return lcl::nilValue();
    }

    lclValueVec* items = new lclValueVec(length);
    for (unsigned int i = 0; i < length; i++) {
        (*items)[i] = lcl::integer(selection_set.at(i));
    }

    id = RS_SCRIPTINGAPI->getNewSelectionId();
    shadowEnv->set(RS_SCRIPTINGAPI->getSelectionName(id), lcl::list(items));

    return lcl::selectionset(id);
}

BUILTIN("sslength")
{
    CHECK_ARGS_IS(1);
    ARG(lclSelectionSet, set);

    return lcl::integer(RS_SCRIPTINGAPI->sslength(set->print(true)));
}

BUILTIN("ssname")
{
    CHECK_ARGS_IS(2);
    ARG(lclSelectionSet, set);
    AG_INT(idx);

    unsigned int id;
    return RS_SCRIPTINGAPI->ssname(set->value(), idx->value(), id)
            ? lcl::ename(id) : lcl::nilValue();
}

BUILTIN("start_dialog")
{
    CHECK_ARGS_IS(0);

    int id =  RS_SCRIPTINGAPI->startDialog();
    return id > -1 ? lcl::integer(id) : lcl::nilValue();
}

BUILTIN("start_image")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);

    return RS_SCRIPTINGAPI->startImage(key->value().c_str()) ? lcl::string(key->value()) : lcl::nilValue();
}

BUILTIN("start_list")
{
    int args = CHECK_ARGS_BETWEEN(1, 3);
    int index = -1, operation = -1;

    ARG(lclString, key);

    if (args > 2)
    {
        AG_INT(op);
        operation = op->value();
    }
    if (args == 3)
    {
        AG_INT(idx);
        index = idx->value();
    }

    return RS_SCRIPTINGAPI->startList(key->value().c_str(),
                                      operation, index) ? lcl::string(key->value()) : lcl::nilValue();
}

BUILTIN("startapp")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    ARG(lclString, com);
    String command = com->value();

    if (count > 1)
    {
        ARG(lclString, para);
        command += " ";
        command += para->value();
    }

    if (system(command.c_str()))
    {
        return lcl::nilValue();
    }
    return lcl::integer(count);
}

BUILTIN("str")
{
    Q_UNUSED(name);
    return lcl::string(printValues(argsBegin, argsEnd, "", false));
}

BUILTIN("strcase")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    ARG(lclString, str);
    String trans = str->value();

    if (count > 1)
    {
        ARG(lclConstant, boolVal);
        if (boolVal->isTrue())
        {
            std::transform(trans.begin(), trans.end(), trans.begin(),
                   [](unsigned char c){ return std::tolower(c); });
            return lcl::string(trans);
        }
    }

    std::transform(trans.begin(), trans.end(), trans.begin(),
                   [](unsigned char c){ return std::toupper(c); });

    return lcl::string(trans);
}

BUILTIN("strlen")
{
    CHECK_ARGS_IS(0);
    return lcl::integer(countValues(argsBegin, argsEnd));
}

BUILTIN("substr")
{
    int count = CHECK_ARGS_AT_LEAST(2);
    ARG(lclString, s);
    AG_INT(start);
    int startPos = (int)start->value();

    if (s)
    {
        String bla = s->value();
        if (startPos > (int)bla.size()+1) {
            startPos = (int)bla.size()+1;
        }

        if (count > 2)
        {
            AG_INT(size);
            return lcl::string(bla.substr(startPos-1, size->value()));
        }
        else
        {
                return lcl::string(bla.substr(startPos-1, bla.size()));
        }
    }

    return lcl::string(String(""));
}

BUILTIN("subst")
{
    CHECK_ARGS_IS(3);
    lclValuePtr newSym = *argsBegin++;
    lclValuePtr oldSym = *argsBegin++;
    ARG(lclSequence, seq);

    const int length = seq->count();
    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());

    for (int i = 0; i < length; i++) {
        if (items->at(i)->print(true) == oldSym->print(true)) {
            items->at(i) = newSym;
            return lcl::list(items);
        }
    }
    return lcl::nilValue();
}

BUILTIN("swap!")
{
    CHECK_ARGS_AT_LEAST(2);
    ARG(lclAtom, atom);

    lclValuePtr op = *argsBegin++; // this gets checked in APPLY

    lclValueVec args(1 + argsEnd - argsBegin);
    args[0] = atom->deref();
    std::copy(argsBegin, argsEnd, args.begin() + 1);

    lclValuePtr value = APPLY(op, args.begin(), args.end());
    return atom->reset(value);
}

BUILTIN("symbol")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, token);
    return lcl::symbol(token->value());
}

BUILTIN("tan")
{
    BUILTIN_FUNCTION(tan);
}

BUILTIN("term_dialog")
{
    CHECK_ARGS_IS(0);

    RS_SCRIPTINGAPI->termDialog();

    return lcl::nilValue();
}

BUILTIN("terpri")
{
    CHECK_ARGS_IS(0);
    std::cout << std::endl;
    return lcl::nilValue();
}

BUILTIN("text_image")
{
    CHECK_ARGS_IS(6);
    AG_INT(x);
    AG_INT(y);
    AG_INT(width);
    AG_INT(height);
    ARG(lclString, text);
    AG_INT(color);

    return RS_SCRIPTINGAPI->textImage(x->value(),
                                      y->value(),
                                      width->value(),
                                      height->value(),
                                      text->value().c_str(),
                                      color->value()) ?  lcl::string(text->value()) : lcl::nilValue();
}

BUILTIN("throw")
{
    CHECK_ARGS_IS(1);
    throw *argsBegin;
}

BUILTIN("time-ms")
{
    CHECK_ARGS_IS(0);

    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(
        high_resolution_clock::now().time_since_epoch()
    );

    return lcl::integer(ms.count());
}

BUILTIN("timeout")
{
    CHECK_ARGS_IS(1);
    AG_INT(timeout);
    SleeperThread::msleep(timeout->value());
    return lcl::nilValue();
}

BUILTIN("type?")
{
    CHECK_ARGS_IS(1);

    if (argsBegin->ptr()->print(true) == "nil")
    {
        return lcl::nilValue();
    }

    return lcl::type(argsBegin->ptr()->type());
}

BUILTIN("unload_dialog")
{
    CHECK_ARGS_IS(1);
    AG_INT(id);

    RS_SCRIPTINGAPI->unloadDialog(id->value());

    return lcl::nilValue();
}


BUILTIN("vals")
{
    CHECK_ARGS_IS(1);
    ARG(lclHash, hash);
    return hash->values();
}

BUILTIN("vec")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, s);
    return lcl::vector(s->begin(), s->end());
}

BUILTIN("vector")
{
    CHECK_ARGS_AT_LEAST(0);
    return lcl::vector(argsBegin, argsEnd);
}

BUILTIN("ver")
{
    CHECK_ARGS_IS(0);
    return lcl::string(Lisp_GetVersion());
}

BUILTIN("vl-consp")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, s);

    if(s->isDotted()) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-directory-files")
{
    int count = CHECK_ARGS_AT_LEAST(0);
    int len = 0;
    String path = "./";
    lclValueVec* items;
    std::vector<std::filesystem::path> sorted_by_name;

    if (count > 0) {
        ARG(lclString, directory);
        path = directory->value();
        if (!std::filesystem::exists(path.c_str())) {
            return lcl::nilValue();
        }
        if (count > 1 && (NIL_PTR || INT_PTR) && !(count == 2 && (NIL_PTR || INT_PTR))) {
            if (NIL_PTR) {
                argsBegin++;
            }
            // pattern + dirs
            AG_INT(directories);
            switch(directories->value())
            {
                case -1:
                    for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                        if (std::filesystem::is_directory(entry.path()))
                        {
                            sorted_by_name.push_back(entry.path().filename());
                            len++;
                        }
                    }
                    break;
                case 0:
                    for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                        sorted_by_name.push_back(entry.path());
                        len++;
                    }
                    break;
                case 1:
                    for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                        if (!std::filesystem::is_directory(entry.path()))
                        {
                            sorted_by_name.push_back(entry.path().filename());
                            len++;
                        }
                    }
                    break;
                default: {}
            }
        }
        else if (count > 1 && !(count == 2 && (NIL_PTR || INT_PTR))) {
            ARG(lclString, pattern);
            int dir = 3;
            if (count > 2) {
                AG_INT(directories2);
                dir = directories2->value();
                if (dir > 1 || dir < -1) {
                    dir = 0;
                }
            }
            // pattern
            bool hasExt = false;
            bool hasName = false;
            String pat = pattern->value();
            int asterix = (int) pat.find_last_of("*");
            if (asterix != -1 && (int) pat.size() >= asterix) {
                hasExt = true;
            }
            if (asterix != -1 && (int) pat.size() >= asterix && pat.size() > pat.substr(asterix+1).size() ) {
                hasName = true;
            }
            for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                if (!std::filesystem::is_directory(entry.path()) &&
                    hasExt &&
                    hasName &&
                    (dir == 3 || dir == 1)) {
                    if ((int)entry.path().filename().string().find(pat.substr(asterix+1)) != -1 &&
                        (int)entry.path().filename().string().find(pat.substr(0, asterix)) != -1) {
                        sorted_by_name.push_back(entry.path().filename());
                        len++;
                    }
                }
                if (!std::filesystem::is_directory(entry.path()) && !hasExt &&
                    (int)entry.path().filename().string().find(pat) != -1 && (dir == 3 || dir == 1)) {
                    sorted_by_name.push_back(entry.path().filename());
                    len++;
                }
                if (std::filesystem::is_directory(entry.path()) && !hasExt &&
                    (int)entry.path().filename().string().find(pat) != -1  && dir == -1) {
                    sorted_by_name.push_back(entry.path().filename());
                    len++;
                }
                if ((int)entry.path().string().find(pat) != -1 && dir == 0) {
                    sorted_by_name.push_back(entry.path());
                    len++;
                }
            }
        }
        else {
            // directory
            for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                if (!std::filesystem::is_directory(entry.path()))
                {
                    sorted_by_name.push_back(entry.path().filename());
                    len++;
                }
            }
        }
    }
    else {
        // current directory
        for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
            if (!std::filesystem::is_directory(entry.path()))
            {
                sorted_by_name.push_back(entry.path().filename());
                len++;
            }
        }
    }
    std::sort(sorted_by_name.begin(), sorted_by_name.end(), compareNatPath);
    items = new lclValueVec(len);
    len = 0;
    for (const auto & filename : sorted_by_name) {
        items->at(len) = lcl::string(filename.string());
        len++;
    }
    return items->size() ? lcl::list(items) : lcl::nilValue();
}

BUILTIN("vl-file-copy")
{
    int count = CHECK_ARGS_AT_LEAST(2);
    ARG(lclString, source);
    ARG(lclString, dest);

    if (count == 3 && argsBegin->ptr()->isTrue()) {

        std::ofstream of;
        std::ios_base::openmode openmode =
            std::ios::ate | std::ios::in | std::ios::binary;
        std::ifstream file(source->value().c_str(), openmode);

        if (file.fail()) {
            return lcl::nilValue();
        }

        String data;
        data.reserve(file.tellg());
        file.seekg(0, std::ios::beg);
        data.append(std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>());

        of.open(dest->value(), std::ios::app);
        if (!of) {
            return lcl::nilValue();
        }
        else {
            of << data;
            of.close();
            return lcl::integer(sizeof source->value());
        }
    }

    std::error_code err;
    std::filesystem::copy(source->value(), dest->value(), std::filesystem::copy_options::update_existing, err);
    if (err) {
        return lcl::nilValue();
    }
    return lcl::integer(sizeof source->value());
}

BUILTIN("vl-file-delete")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }
    if (std::filesystem::remove(path->value().c_str()))
    {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-file-directory-p")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    const std::filesystem::directory_entry dir(path->value().c_str());
    if (std::filesystem::exists(path->value().c_str()) &&
        dir.is_directory()) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-file-rename")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, path);
    ARG(lclString, newName);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }
    std::error_code err;
    std::filesystem::rename(path->value().c_str(), newName->value().c_str(), err);
    if (err) {
        return lcl::nilValue();
    }
    return lcl::trueValue();
}

BUILTIN("vl-file-size")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }
    if (!std::filesystem::is_directory(path->value().c_str())) {
        return lcl::string("0");
    }
    try {
        [[maybe_unused]] auto size = std::filesystem::file_size(path->value().c_str());
        char str[50];

#ifdef _MSC_VER
        snprintf(str, 50, "%llu", size);
#else
        sprintf(str, "%lu", size);
        snprintf(str, 50, "%lu", size);
#endif
        return lcl::string(str);
    }
    catch (std::filesystem::filesystem_error&) {}
    return lcl::nilValue();
}

BUILTIN("vl-file-systime")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }

    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(path->value().c_str());
    std::time_t cftime = to_time_t(ftime); // assuming system_clock

    char buffer[64];
    int J,M,W,D,h,m,s;

    if (strftime(buffer, sizeof buffer, "%Y %m %w %e %I %M %S", std::localtime(&cftime))) {
        sscanf (buffer,"%d %d %d %d %d %d %d",&J,&M,&W,&D,&h,&m,&s);

        lclValueVec* items = new lclValueVec(6);
        items->at(0) = new lclInteger(J);
        items->at(1) = new lclInteger(M);
        items->at(2) = new lclInteger(W);
        items->at(3) = new lclInteger(D);
        items->at(4) = new lclInteger(m);
        items->at(5) = new lclInteger(s);
        return lcl::list(items);
    }
    return lcl::nilValue();
}

BUILTIN("vl-filename-base")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    const std::filesystem::path p(path->value());
    return lcl::string(p.stem().string());
}

BUILTIN("vl-filename-directory")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    const std::filesystem::path p(path->value());
    if (!p.has_extension()) {
        return lcl::string(path->value());
    }

    const auto directory = std::filesystem::path{ p }.parent_path().string();
    return lcl::string(directory);
}

BUILTIN("vl-filename-extension")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    const std::filesystem::path p(path->value());
    if (!p.has_extension()) {
        return lcl::nilValue();
    }

    return lcl::string(p.extension().string());
}

BUILTIN("vl-filename-mktemp")
{
    int count = CHECK_ARGS_AT_LEAST(0);
    char num[4];
    snprintf(num, 4, "%03x", ++tmpFileCount);

    String filename = "tmpfile_";
    std::filesystem::path path("");
    std::filesystem::path p(std::filesystem::temp_directory_path());
    std::filesystem::path d("");

    filename +=  + num;
    path = p / filename;

    if (count > 0) {
        ARG(lclString, pattern);
        p = pattern->value().c_str();
        filename = p.stem().string();
        filename +=  + num;
        if (!p.has_root_path()) {
            path = std::filesystem::temp_directory_path() / d;
        }
        else {
            path = p.root_path() / p.relative_path().remove_filename();
        }
        if (p.has_extension()) {
            filename += p.extension().string();
        }
        path += filename;
    }
    if (count > 1) {
        ARG(lclString, directory);
        path = directory->value() / d;
        path += filename;
    }
    if (count == 3) {
        ARG(lclString, extension);
        path += extension->value();
    }
    return lcl::string(path.string());
}

BUILTIN("vl-mkdir")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, dir);

    if(std::filesystem::create_directory(dir->value())) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-position")
{
    CHECK_ARGS_IS(2);
    lclValuePtr op = *argsBegin++; // this gets checked in APPLY

    const lclSequence* seq = VALUE_CAST(lclSequence, *(argsBegin));
    const int length = seq->count();
    for (int i = 0; i < length; i++) {
        if(seq->item(i)->print(true) == op->print(true)) {
            return lcl::integer(i);
        }
    }
    return lcl::nilValue();
}

BUILTIN("slide_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(width);
    AG_INT(height);
    ARG(lclString, filename);

    return RS_SCRIPTINGAPI->slideImage(x1->value(),
                                       y1->value(),
                                       width->value(),
                                       height->value(),
                                       filename->value().c_str()) ? lcl::string(filename->value()) : lcl::nilValue();

}

BUILTIN("symbol")
{
    CHECK_ARGS_IS(1);
    if(argsBegin->ptr()->type() == LCLTYPE::SYM) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vector_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(x2);
    AG_INT(y2);
    AG_INT(color);

    return RS_SCRIPTINGAPI->vectorImage(x1->value(),
                                        y1->value(),
                                        x2->value(),
                                        y2->value(),
                                        color->value()) ? lcl::integer(color->value()) : lcl::nilValue();
}

BUILTIN("wcmatch")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, str);
    ARG(lclString, p);
    std::vector<String> StringList;
    String del = ",";
    String pat = p->value();
    auto pos = pat.find(del);

    while (pos != String::npos) {
        StringList.push_back(pat.substr(0, pos));
        pat.erase(0, pos + del.length());
        pos = pat.find(del);
    }
    StringList.push_back(pat);
    for (auto &it : StringList) {
        String pattern = it;
        String expr = "";
        bool exclude = false;
        bool open_br = false;
        for (auto &ch : it) {
            switch (ch) {
                case '#':
                    expr += "(\\d)";
                    break;
                case '@':
                    expr += "[A-Za-zÀ-ȕ]";
                    break;
                case ' ':
                    expr += "[ ]+";
                    break;
                case '.':
                    expr += "([^(A-Za-z0-9 )]{1,})";
                    break;
                case '*':
                    expr += "(.*)";
                    break;
                case '?':
                    expr += "[A-Za-zÀ-ȕ0-9_ ]";
                    break;
                case '~': {
                    if (open_br) {
                        expr += "^";
                    } else {
                        expr += "[^";
                    exclude = true;
                    }
                    break;
                }
                case '[':
                    expr += "[";
                    open_br = true;
                    break;
                case ']': {
                    expr += "]{1}";
                    open_br = false;
                    break;
                }
                case '`':
                    expr += "//";
                    break;
                default: {
                    expr += ch;
                }
            }
        }
        if (exclude) {
            expr += "]*";
        }
        std::regex e (expr);
        if (std::regex_match (str->value(),e)) {
            return lcl::trueValue();
        }
    }
    return lcl::nilValue();
}

BUILTIN("with-meta")
{
    CHECK_ARGS_IS(2);
    lclValuePtr obj  = *argsBegin++;
    lclValuePtr meta = *argsBegin++;
    return obj->withMeta(meta);
}

BUILTIN("write-line")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    //multi
    ARG(lclString, str);

    if (count == 1)
    {
        return lcl::string(str->value());
    }

    ARG(lclFile, pf);

    return pf->writeLine(str->value());
}

BUILTIN("write-char")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    AG_INT(c);

    std::cout << itoa64(c->value()) << std::endl;

    if (count == 1)
    {
        return lcl::integer(c->value());
    }

    ARG(lclFile, pf);

    return pf->writeChar(itoa64(c->value()));
}

BUILTIN("zero?")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() == 0.0);
    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() == 0);
    }
    return lcl::falseValue();
}

BUILTIN("zerop")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return val->value() == 0 ? lcl::trueValue() : lcl::nilValue();
    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return val->value() == 0 ? lcl::trueValue() : lcl::nilValue();
    }
    return lcl::nilValue();
}

static lclValuePtr entget(lclEname *en)
{
    //RS_EntityContainer* entityContainer = RS_SCRIPTINGAPI->getContainer();

    RS_Graphic *graphic = RS_SCRIPTINGAPI->getGraphic();

    if(!graphic)
    {
        return lcl::nilValue();
    }

#if 0
    if(entityContainer->count())
    {
#endif
        for (RS_Entity *e = graphic->firstEntity(RS2::ResolveNone);
            e; e = graphic->nextEntity(RS2::ResolveNone)) {
            if ( !(e->getFlag(RS2::FlagUndone)) && e->getId() == en->value())
            {
                RS_Pen pen = e->getPen(false);

                lclValueVec *entity = new lclValueVec;

                lclValueVec *ename = new lclValueVec(3);
                ename->at(0) = lcl::integer(-1);
                ename->at(1) = lcl::symbol(".");
                ename->at(2) = lcl::ename(en->value());

                lclValueVec *ebname = new lclValueVec(3);
                ebname->at(0) = lcl::integer(330);
                ebname->at(1) = lcl::symbol(".");
                ebname->at(2) = lcl::ename(e->getParent()->getId());
                entity->push_back(lcl::list(ebname));

                lclValueVec *handle = new lclValueVec(3);
                handle->at(0) = lcl::integer(5);
                handle->at(1) = lcl::symbol(".");
                handle->at(2) = lcl::string(en->valueStr());
                entity->push_back(lcl::list(handle));

                enum RS2::LineType lineType = pen.getLineType();
                if(lineType != RS2::LineByLayer)
                {
                    lclValueVec *lType = new lclValueVec(3);
                    lType->at(0) = lcl::integer(6);
                    lType->at(1) = lcl::symbol(".");
                    lType->at(2) = lcl::string(RS_FilterDXFRW::lineTypeToName(pen.getLineType()).toStdString());
                    entity->push_back(lcl::list(lType));
                }

                enum RS2::LineWidth lineWidth = pen.getWidth();
                if(lineWidth != RS2::WidthByLayer)
                {
                    lclValueVec *lWidth = new lclValueVec(3);
                    lWidth->at(0) = lcl::integer(48);
                    lWidth->at(1) = lcl::symbol(".");

                    int width = static_cast<int>(lineWidth);
                    if (width < 0)
                    {
                        lWidth->at(2) = lcl::ldouble(1.0);
                    }
                    else
                    {
                        lWidth->at(2) = lcl::ldouble(double(width) / 100.0);
                    }
                    entity->push_back(lcl::list(lWidth));

                }

                RS_Color color = pen.getColor();
                if(!color.isByLayer())
                {
                    int exact_rgb;
                    lclValueVec *col = new lclValueVec(3);
                    col->at(0) = lcl::integer(62);
                    col->at(1) = lcl::symbol(".");
                    col->at(2) = lcl::integer(RS_FilterDXFRW::colorToNumber(color, &exact_rgb));
                    entity->push_back(lcl::list(col));

                    if (exact_rgb >= 0)
                    {
                        lclValueVec *col = new lclValueVec(3);
                        col->at(0) = lcl::integer(420);
                        col->at(1) = lcl::symbol(".");
                        col->at(2) = lcl::integer(exact_rgb);
                        entity->push_back(lcl::list(col));
                    }
                }

                lclValueVec *acdb = new lclValueVec(3);
                acdb->at(0) = lcl::integer(100);
                acdb->at(1) = lcl::symbol(".");
                acdb->at(2) = lcl::string("AcDbEntity");

                lclValueVec *mspace = new lclValueVec(3);
                mspace->at(0) = lcl::integer(67);
                mspace->at(1) = lcl::symbol(".");
                mspace->at(2) = lcl::integer(0);

                lclValueVec *layoutTabName = new lclValueVec(3);
                layoutTabName->at(0) = lcl::integer(410);
                layoutTabName->at(1) = lcl::symbol(".");
                layoutTabName->at(2) = lcl::string("Model");

                lclValueVec *layer = new lclValueVec(3);
                layer->at(0) = lcl::integer(8);
                layer->at(1) = lcl::symbol(".");
                layer->at(2) = lcl::string(e->getLayer()->getName().toStdString());

                lclValueVec *extrDir = new lclValueVec(4);
                extrDir->at(0) = lcl::integer(210);
                extrDir->at(1) = lcl::ldouble(0.0);
                extrDir->at(2) = lcl::ldouble(0.0);
                extrDir->at(3) = lcl::ldouble(1.0);

                entity->push_back(lcl::list(acdb));
                entity->push_back(lcl::list(mspace));
                entity->push_back(lcl::list(layoutTabName));
                entity->push_back(lcl::list(layer));

                switch (e->rtti())
                {
                    case RS2::EntityPoint:
                    {
                        RS_Point* p = (RS_Point*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("POINT");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbPoint");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(p->getPos().x);
                        pnt->at(2) = lcl::ldouble(p->getPos().y);
                        pnt->at(3) = lcl::ldouble(p->getPos().z);
                        entity->push_back(lcl::list(pnt));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityLine:
                    {
                        RS_Line* l = (RS_Line*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("LINE");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbLine");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *startpnt = new lclValueVec(4);
                        startpnt->at(0) = lcl::integer(10);
                        startpnt->at(1) = lcl::ldouble(l->getStartpoint().x);
                        startpnt->at(2) = lcl::ldouble(l->getStartpoint().y);
                        startpnt->at(3) = lcl::ldouble(l->getStartpoint().z);
                        entity->push_back(lcl::list(startpnt));

                        lclValueVec *endpnt = new lclValueVec(4);
                        endpnt->at(0) = lcl::integer(11);
                        endpnt->at(1) = lcl::ldouble(l->getEndpoint().x);
                        endpnt->at(2) = lcl::ldouble(l->getEndpoint().y);
                        endpnt->at(3) = lcl::ldouble(l->getEndpoint().z);
                        entity->push_back(lcl::list(endpnt));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityCircle:
                    {
                        RS_Circle* c = (RS_Circle*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("CIRCLE");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbCircle");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *center = new lclValueVec(4);
                        center->at(0) = lcl::integer(10);
                        center->at(1) = lcl::ldouble(c->getCenter().x);
                        center->at(2) = lcl::ldouble(c->getCenter().y);
                        center->at(3) = lcl::ldouble(c->getCenter().z);
                        entity->push_back(lcl::list(center));

                        lclValueVec *radius = new lclValueVec(3);
                        radius->at(0) = lcl::integer(40);
                        radius->at(1) = lcl::symbol(".");
                        radius->at(2) = lcl::ldouble(c->getRadius());
                        entity->push_back(lcl::list(radius));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityArc:
                    {
                        RS_Arc* a = (RS_Arc*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("ARC");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbArc");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *center = new lclValueVec(4);
                        center->at(0) = lcl::integer(10);
                        center->at(1) = lcl::ldouble(a->getCenter().x);
                        center->at(2) = lcl::ldouble(a->getCenter().y);
                        center->at(3) = lcl::ldouble(a->getCenter().z);
                        entity->push_back(lcl::list(center));

                        lclValueVec *radius = new lclValueVec(3);
                        radius->at(0) = lcl::integer(40);
                        radius->at(1) = lcl::symbol(".");
                        radius->at(2) = lcl::ldouble(a->getRadius());
                        entity->push_back(lcl::list(radius));

                        lclValueVec *startAngle = new lclValueVec(3);
                        startAngle->at(0) = lcl::integer(50);
                        startAngle->at(1) = lcl::symbol(".");
                        startAngle->at(2) = lcl::ldouble(a->isReversed() ? a->getAngle2() : a->getAngle1());
                        entity->push_back(lcl::list(startAngle));

                        lclValueVec *endAngle = new lclValueVec(3);
                        endAngle->at(0) = lcl::integer(51);
                        endAngle->at(1) = lcl::symbol(".");
                        endAngle->at(2) = lcl::ldouble(a->isReversed() ? a->getAngle1() : a->getAngle2());
                        entity->push_back(lcl::list(endAngle));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntitySolid:
                    {
                        RS_Solid* sol = (RS_Solid*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("SOLID");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbTrace");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *cor = new lclValueVec(4);
                        cor->at(0) = lcl::integer(10);
                        cor->at(1) = lcl::ldouble(sol->getCorner(0).x);
                        cor->at(2) = lcl::ldouble(sol->getCorner(0).y);
                        cor->at(3) = lcl::ldouble(sol->getCorner(0).z);
                        entity->push_back(lcl::list(cor));

                        lclValueVec *cor1 = new lclValueVec(4);
                        cor1->at(0) = lcl::integer(11);
                        cor1->at(1) = lcl::ldouble(sol->getCorner(1).x);
                        cor1->at(2) = lcl::ldouble(sol->getCorner(1).y);
                        cor1->at(3) = lcl::ldouble(sol->getCorner(1).z);
                        entity->push_back(lcl::list(cor1));

                        lclValueVec *cor2 = new lclValueVec(4);
                        cor2->at(0) = lcl::integer(12);
                        cor2->at(1) = lcl::ldouble(sol->getCorner(2).x);
                        cor2->at(2) = lcl::ldouble(sol->getCorner(2).y);
                        cor2->at(3) = lcl::ldouble(sol->getCorner(2).z);
                        entity->push_back(lcl::list(cor2));

                        if (!sol->isTriangle())
                        {
                            lclValueVec *cor3 = new lclValueVec(4);
                            cor3->at(0) = lcl::integer(13);
                            cor3->at(1) = lcl::ldouble(sol->getCorner(3).x);
                            cor3->at(2) = lcl::ldouble(sol->getCorner(3).y);
                            cor3->at(3) = lcl::ldouble(sol->getCorner(3).z);
                            entity->push_back(lcl::list(cor3));
                        }

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityEllipse:
                    {
                        RS_Ellipse* ellipse=static_cast<RS_Ellipse*>(e);

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("ELLIPSE");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbEllipse");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *center = new lclValueVec(4);
                        center->at(0) = lcl::integer(10);
                        center->at(1) = lcl::ldouble(ellipse->getCenter().x);
                        center->at(2) = lcl::ldouble(ellipse->getCenter().y);
                        center->at(3) = lcl::ldouble(ellipse->getCenter().z);
                        entity->push_back(lcl::list(center));

                        lclValueVec *majorpnt = new lclValueVec(4);
                        majorpnt->at(0) = lcl::integer(11);
                        majorpnt->at(1) = lcl::ldouble(ellipse->getMajorP().x);
                        majorpnt->at(2) = lcl::ldouble(ellipse->getMajorP().y);
                        majorpnt->at(3) = lcl::ldouble(ellipse->getMajorP().z);
                        entity->push_back(lcl::list(majorpnt));

                        entity->push_back(lcl::list(extrDir));

                        lclValueVec *ratio = new lclValueVec(3);
                        ratio->at(0) = lcl::integer(40);
                        ratio->at(1) = lcl::symbol(".");
                        ratio->at(2) = lcl::ldouble(ellipse->getRatio());
                        entity->push_back(lcl::list(ratio));

                        lclValueVec *angle1 = new lclValueVec(3);
                        angle1->at(0) = lcl::integer(41);
                        angle1->at(1) = lcl::symbol(".");

                        if (ellipse->isReversed()) {
                            angle1->at(2) = lcl::ldouble(ellipse->getData().angle2);
                        } else {
                            angle1->at(2) = lcl::ldouble(ellipse->getData().angle1);
                        }

                        entity->push_back(lcl::list(angle1));

                        lclValueVec *angle2 = new lclValueVec(3);
                        angle2->at(0) = lcl::integer(42);
                        angle2->at(1) = lcl::symbol(".");

                        if (ellipse->isReversed()) {
                            angle2->at(2) = lcl::ldouble(ellipse->getData().angle1);
                        } else {
                            angle2->at(2) = lcl::ldouble(ellipse->getData().angle2);
                        }
                        entity->push_back(lcl::list(angle2));
                        break;
                    }

                    case RS2::EntityPolyline:
                    {
                        RS_Polyline* pl = (RS_Polyline*)e;
                        bool is3d = false;

                        for (auto &v : pl->getRefPoints())
                        {
                            if(v.z != 0.0)
                            {
                                is3d = true;
                                break;
                            }
                        }

                        if (is3d)
                        {
                            lclValueVec *name = new lclValueVec(3);
                            name->at(0) = lcl::integer(0);
                            name->at(1) = lcl::symbol(".");
                            name->at(2) = lcl::string("POLYLINE");
                            entity->insert(entity->begin(), lcl::list(name));
                            entity->insert(entity->begin(), lcl::list(ename));

                            lclValueVec *acdbL = new lclValueVec(3);
                            acdbL->at(0) = lcl::integer(100);
                            acdbL->at(1) = lcl::symbol(".");
                            acdbL->at(2) = lcl::string("AcDb3dPolyline");
                            entity->push_back(lcl::list(acdbL));

                            lclValueVec *flag = new lclValueVec(3);
                            flag->at(0) = lcl::integer(70);
                            flag->at(1) = lcl::symbol(".");
                            int fl = 0;
                            fl |= 8;
                            if (pl->isClosed())
                            {
                                fl |= 1;
                            }
                            flag->at(2) = lcl::integer(fl);
                            entity->push_back(lcl::list(flag));

                            entity->push_back(lcl::list(extrDir));
                        }
                        else
                        {
                            lclValueVec *name = new lclValueVec(3);
                            name->at(0) = lcl::integer(0);
                            name->at(1) = lcl::symbol(".");
                            name->at(2) = lcl::string("LWPOLYLINE");
                            entity->insert(entity->begin(), lcl::list(name));
                            entity->insert(entity->begin(), lcl::list(ename));

                            lclValueVec *acdbL = new lclValueVec(3);
                            acdbL->at(0) = lcl::integer(100);
                            acdbL->at(1) = lcl::symbol(".");
                            acdbL->at(2) = lcl::string("AcDbPolyline");
                            entity->push_back(lcl::list(acdbL));

                            for (auto &v : pl->getRefPoints())
                            {
                                lclValueVec *pnt = new lclValueVec(3);
                                pnt->at(0) = lcl::integer(10);
                                pnt->at(1) = lcl::ldouble(v.x);
                                pnt->at(2) = lcl::ldouble(v.y);
                                entity->push_back(lcl::list(pnt));

                                lclValueVec *startWidth = new lclValueVec(3);
                                startWidth->at(0) = lcl::integer(40);
                                startWidth->at(1) = lcl::symbol(".");
                                startWidth->at(2) = lcl::ldouble(0.0);
                                entity->push_back(lcl::list(startWidth));

                                lclValueVec *endWidth = new lclValueVec(3);
                                endWidth->at(0) = lcl::integer(41);
                                endWidth->at(1) = lcl::symbol(".");
                                endWidth->at(2) = lcl::ldouble(0.0);
                                entity->push_back(lcl::list(endWidth));

                                lclValueVec *bulgeWidth = new lclValueVec(3);
                                bulgeWidth->at(0) = lcl::integer(42);
                                bulgeWidth->at(1) = lcl::symbol(".");
                                bulgeWidth->at(2) = lcl::ldouble(0.0);
                                entity->push_back(lcl::list(bulgeWidth));
                            }

                            if (pl->isClosed())
                            {
                                lclValueVec *flag = new lclValueVec(3);
                                flag->at(0) = lcl::integer(70);
                                flag->at(1) = lcl::symbol(".");
                                flag->at(2) = lcl::integer(1);
                                entity->push_back(lcl::list(flag));
                            }

                            entity->push_back(lcl::list(extrDir));
                        }
                        break;
                    }

                    case RS2::EntitySpline:
                    {
                        RS_Spline* spl = (RS_Spline*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("SPLINE");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbSpline");
                        entity->push_back(lcl::list(acdbL));

                        entity->push_back(lcl::list(extrDir));

                        lclValueVec *flag = new lclValueVec(3);
                        flag->at(0) = lcl::integer(70);
                        flag->at(1) = lcl::symbol(".");
                        flag->at(2) = lcl::integer(spl->isClosed() ? 1 : 8);
                        entity->push_back(lcl::list(flag));

                        lclValueVec *degree = new lclValueVec(3);
                        degree->at(0) = lcl::integer(71);
                        degree->at(1) = lcl::symbol(".");
                        degree->at(2) = lcl::integer(spl->getDegree());
                        entity->push_back(lcl::list(degree));

                        lclValueVec *numKnots = new lclValueVec(3);
                        numKnots->at(0) = lcl::integer(72);
                        numKnots->at(1) = lcl::symbol(".");
                        numKnots->at(2) = lcl::integer(spl->getNumberOfKnots());
                        entity->push_back(lcl::list(numKnots));

                        lclValueVec *numCtrlPnts = new lclValueVec(3);
                        numCtrlPnts->at(0) = lcl::integer(73);
                        numCtrlPnts->at(1) = lcl::symbol(".");
                        numCtrlPnts->at(2) = lcl::integer(static_cast<int>(spl->getNumberOfControlPoints()));
                        entity->push_back(lcl::list(numCtrlPnts));

                        // value missing ad dummy
                        lclValueVec *numFitPnts = new lclValueVec(3);
                        numFitPnts->at(0) = lcl::integer(74);
                        numFitPnts->at(1) = lcl::symbol(".");
                        numFitPnts->at(2) = lcl::integer(0);
                        entity->push_back(lcl::list(numFitPnts));

                        // value from dxf file result
                        lclValueVec *knotTolerance = new lclValueVec(3);
                        knotTolerance->at(0) = lcl::integer(42);
                        knotTolerance->at(1) = lcl::symbol(".");
                        knotTolerance->at(2) = lcl::ldouble(1e-07);
                        entity->push_back(lcl::list(knotTolerance));

                        lclValueVec *ControlpointTolerance = new lclValueVec(3);
                        ControlpointTolerance->at(0) = lcl::integer(43);
                        ControlpointTolerance->at(1) = lcl::symbol(".");
                        ControlpointTolerance->at(2) = lcl::ldouble(1e-07);
                        entity->push_back(lcl::list(ControlpointTolerance));

                        lclValueVec *fitTolerance = new lclValueVec(3);
                        fitTolerance->at(0) = lcl::integer(44);
                        fitTolerance->at(1) = lcl::symbol(".");
                        fitTolerance->at(2) = lcl::ldouble(1e-07);
                        entity->push_back(lcl::list(fitTolerance));

                        for (auto &v : spl->getRefPoints())
                        {
                            lclValueVec *pnt = new lclValueVec(4);
                            pnt->at(0) = lcl::integer(10);
                            pnt->at(1) = lcl::ldouble(v.x);
                            pnt->at(2) = lcl::ldouble(v.y);
                            pnt->at(3) = lcl::ldouble(v.z);
                            entity->push_back(lcl::list(pnt));
                        }

                        qDebug() << "knotslist.size() >>>>>>>>" << spl->getData().knotslist.size();

                        for (auto &k : spl->getData().knotslist)
                        {
                            lclValueVec *knot = new lclValueVec(3);
                            knot->at(0) = lcl::integer(40);
                            knot->at(1) = lcl::symbol(".");
                            knot->at(2) = lcl::ldouble(k);
                            entity->push_back(lcl::list(knot));
                        }
                        break;
                    }
                    case RS2::EntitySplinePoints:
                    case RS2::EntityParabola:
                        // not implemented yet
                        delete ename;
                        delete handle;
                        delete acdb;
                        delete mspace;
                        delete layoutTabName;
                        delete layer;
                        delete extrDir;

                        return lcl::nilValue();
                        break;
                        break;

                    case RS2::EntityInsert:
                    {
                        RS_Insert* i = (RS_Insert*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("INSERT");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbBlockReference");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *blockName = new lclValueVec(3);
                        blockName->at(0) = lcl::integer(2);
                        blockName->at(1) = lcl::symbol(".");
                        blockName->at(2) = lcl::string(i->getName().toStdString());
                        entity->push_back(lcl::list(blockName));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(i->getInsertionPoint().x);
                        pnt->at(2) = lcl::ldouble(i->getInsertionPoint().y);
                        pnt->at(3) = lcl::ldouble(i->getInsertionPoint().z);
                        entity->push_back(lcl::list(pnt));

                        lclValueVec *scaleX = new lclValueVec(3);
                        scaleX->at(0) = lcl::integer(41);
                        scaleX->at(1) = lcl::symbol(".");
                        scaleX->at(2) = lcl::ldouble(i->getScale().x);
                        entity->push_back(lcl::list(scaleX));

                        lclValueVec *scaleY = new lclValueVec(3);
                        scaleY->at(0) = lcl::integer(42);
                        scaleY->at(1) = lcl::symbol(".");
                        scaleY->at(2) = lcl::ldouble(i->getScale().y);
                        entity->push_back(lcl::list(scaleY));

                        lclValueVec *radius = new lclValueVec(3);
                        radius->at(0) = lcl::integer(50);
                        radius->at(1) = lcl::symbol(".");
                        radius->at(2) = lcl::ldouble(i->getRadius());
                        entity->push_back(lcl::list(radius));

                        lclValueVec *columnCount = new lclValueVec(3);
                        columnCount->at(0) = lcl::integer(70);
                        columnCount->at(1) = lcl::symbol(".");
                        columnCount->at(2) = lcl::ldouble(i->getCols());
                        entity->push_back(lcl::list(columnCount));

                        lclValueVec *rowCount = new lclValueVec(3);
                        rowCount->at(0) = lcl::integer(71);
                        rowCount->at(1) = lcl::symbol(".");
                        rowCount->at(2) = lcl::ldouble(i->getRows());
                        entity->push_back(lcl::list(rowCount));

                        lclValueVec *columnSpacing = new lclValueVec(3);
                        columnSpacing->at(0) = lcl::integer(44);
                        columnSpacing->at(1) = lcl::symbol(".");
                        columnSpacing->at(2) = lcl::ldouble(i->getSpacing().y);
                        entity->push_back(lcl::list(columnSpacing));

                        lclValueVec *rowSpacing = new lclValueVec(3);
                        rowSpacing->at(0) = lcl::integer(45);
                        rowSpacing->at(1) = lcl::symbol(".");
                        rowSpacing->at(2) = lcl::ldouble(i->getSpacing().x);
                        entity->push_back(lcl::list(rowSpacing));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityMText:
                    {
                        RS_MText* t = (RS_MText*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("MTEXT");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbMText");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(t->getInsertionPoint().x);
                        pnt->at(2) = lcl::ldouble(t->getInsertionPoint().y);
                        pnt->at(3) = lcl::ldouble(t->getInsertionPoint().z);
                        entity->push_back(lcl::list(pnt));

                        lclValueVec *textHeight = new lclValueVec(3);
                        textHeight->at(0) = lcl::integer(40);
                        textHeight->at(1) = lcl::symbol(".");
                        textHeight->at(2) = lcl::ldouble(t->getUsedTextHeight());
                        entity->push_back(lcl::list(textHeight));

                        lclValueVec *width = new lclValueVec(3);
                        width->at(0) = lcl::integer(41);
                        width->at(1) = lcl::symbol(".");
                        width->at(2) = lcl::ldouble(t->getWidth());
                        entity->push_back(lcl::list(width));

                        lclValueVec *alignment = new lclValueVec(3);
                        alignment->at(0) = lcl::integer(71);
                        alignment->at(1) = lcl::symbol(".");
                        alignment->at(2) = lcl::integer(t->getAlignment());
                        entity->push_back(lcl::list(alignment));

                        lclValueVec *drawingDirection = new lclValueVec(3);
                        drawingDirection->at(0) = lcl::integer(72);
                        drawingDirection->at(1) = lcl::symbol(".");

                        RS_MTextData::MTextDrawingDirection dir = t->getDrawingDirection();
                        unsigned int dxfDir = 5;

                        switch(dir)
                        {
                            case RS_MTextData::MTextDrawingDirection::LeftToRight:
                                dxfDir = 1;
                                break;
                            case RS_MTextData::MTextDrawingDirection::RightToLeft:
                                dxfDir = 2;
                                break;
                            case RS_MTextData::MTextDrawingDirection::TopToBottom:
                                dxfDir = 3;
                                break;
                            default:
                                dxfDir = 5;
                        }

                        drawingDirection->at(2) = lcl::integer(dxfDir);
                        entity->push_back(lcl::list(drawingDirection));

                        lclValueVec *text = new lclValueVec(3);
                        text->at(0) = lcl::integer(1);
                        text->at(1) = lcl::symbol(".");
                        text->at(2) = lcl::string(t->getText().toStdString());
                        entity->push_back(lcl::list(text));

                        lclValueVec *style = new lclValueVec(3);
                        style->at(0) = lcl::integer(7);
                        style->at(1) = lcl::symbol(".");
                        style->at(2) = lcl::string(t->getStyle().toStdString());
                        entity->push_back(lcl::list(style));

                        lclValueVec *height = new lclValueVec(3);
                        height->at(0) = lcl::integer(43);
                        height->at(1) = lcl::symbol(".");
                        height->at(2) = lcl::ldouble(t->getHeight());
                        entity->push_back(lcl::list(height));

                        lclValueVec *angle = new lclValueVec(3);
                        angle->at(0) = lcl::integer(50);
                        angle->at(1) = lcl::symbol(".");
                        angle->at(2) = lcl::ldouble(t->getAngle()*180/M_PI);
                        entity->push_back(lcl::list(angle));

                        lclValueVec *lineSpacingStyle = new lclValueVec(3);
                        lineSpacingStyle->at(0) = lcl::integer(73);
                        lineSpacingStyle->at(1) = lcl::symbol(".");
                        lineSpacingStyle->at(2) = lcl::integer(t->getLineSpacingStyle() == RS_MTextData::MTextLineSpacingStyle::AtLeast ? 1 : 2);
                        entity->push_back(lcl::list(lineSpacingStyle));

                        lclValueVec *textSpace = new lclValueVec(3);
                        textSpace->at(0) = lcl::integer(44);
                        textSpace->at(1) = lcl::symbol(".");
                        textSpace->at(2) = lcl::ldouble(t->getLineSpacingFactor());
                        entity->push_back(lcl::list(textSpace));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityText:
                    {
                        RS_Text* t = (RS_Text*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("TEXT");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbText");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(t->getInsertionPoint().x);
                        pnt->at(2) = lcl::ldouble(t->getInsertionPoint().y);
                        pnt->at(3) = lcl::ldouble(t->getInsertionPoint().z);
                        entity->push_back(lcl::list(pnt));

                        lclValueVec *height = new lclValueVec(3);
                        height->at(0) = lcl::integer(40);
                        height->at(1) = lcl::symbol(".");
                        height->at(2) = lcl::ldouble(t->getHeight());
                        entity->push_back(lcl::list(height));

                        lclValueVec *text = new lclValueVec(3);
                        text->at(0) = lcl::integer(1);
                        text->at(1) = lcl::symbol(".");
                        text->at(2) = lcl::string(t->getText().toStdString());
                        entity->push_back(lcl::list(text));

                        lclValueVec *angle = new lclValueVec(3);
                        angle->at(0) = lcl::integer(50);
                        angle->at(1) = lcl::symbol(".");
                        angle->at(2) = lcl::ldouble(t->getAngle()*180/M_PI);
                        entity->push_back(lcl::list(angle));

                        lclValueVec *scaleFactor = new lclValueVec(3);
                        scaleFactor->at(0) = lcl::integer(41);
                        scaleFactor->at(1) = lcl::symbol(".");
                        scaleFactor->at(2) = lcl::ldouble(t->getWidthRel());
                        entity->push_back(lcl::list(scaleFactor));

                        lclValueVec *style = new lclValueVec(3);
                        style->at(0) = lcl::integer(7);
                        style->at(1) = lcl::symbol(".");
                        style->at(2) = lcl::string(t->getStyle().toStdString());
                        entity->push_back(lcl::list(style));

                        lclValueVec *textGen = new lclValueVec(3);
                        textGen->at(0) = lcl::integer(71);
                        textGen->at(1) = lcl::symbol(".");
                        textGen->at(2) = lcl::integer(static_cast<int>(t->getTextGeneration()));
                        entity->push_back(lcl::list(textGen));

                        lclValueVec *hTextJust = new lclValueVec(3);
                        hTextJust->at(0) = lcl::integer(72);
                        hTextJust->at(1) = lcl::symbol(".");
                        hTextJust->at(2) = lcl::integer(static_cast<int>(t->getHAlign()));
                        entity->push_back(lcl::list(hTextJust));

                        lclValueVec *pnt2 = new lclValueVec(4);
                        pnt2->at(0) = lcl::integer(11);

                        if (t->getVAlign() != RS_TextData::VABaseline || t->getHAlign() != RS_TextData::HALeft)
                        {
                            if (t->getHAlign() == RS_TextData::HAAligned || t->getHAlign() == RS_TextData::HAFit)
                            {
                                pnt2->at(1) = lcl::ldouble(t->getInsertionPoint().x);
                                pnt2->at(2) = lcl::ldouble(t->getInsertionPoint().y);
                                pnt2->at(3) = lcl::ldouble(t->getInsertionPoint().z);
                            }
                            else
                            {
                                pnt2->at(1) = lcl::ldouble(t->getInsertionPoint().x);
                                pnt2->at(2) = lcl::ldouble(t->getInsertionPoint().y);
                                pnt2->at(3) = lcl::ldouble(t->getInsertionPoint().z);
                            }
                        }
                        else
                        {
                            pnt2->at(1) = lcl::ldouble(0.0);
                            pnt2->at(2) = lcl::ldouble(0.0);
                            pnt2->at(3) = lcl::ldouble(0.0);
                        }

                        entity->push_back(lcl::list(pnt2));

                        lclValueVec *vTextJust = new lclValueVec(3);
                        vTextJust->at(0) = lcl::integer(73);
                        vTextJust->at(1) = lcl::symbol(".");
                        vTextJust->at(2) = lcl::integer(static_cast<int>(t->getVAlign()));
                        entity->push_back(lcl::list(vTextJust));

                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    case RS2::EntityDimLinear:
                    case RS2::EntityDimAligned:
                    case RS2::EntityDimAngular:
                    case RS2::EntityDimRadial:
                    case RS2::EntityDimDiametric:
                    {
                        RS_Dimension* dim = (RS_DimDiametric*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("DIMENSION");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbDimension");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *defPnt = new lclValueVec(4);
                        defPnt->at(0) = lcl::integer(10);
                        defPnt->at(1) = lcl::ldouble(dim->getDefinitionPoint().x);
                        defPnt->at(2) = lcl::ldouble(dim->getDefinitionPoint().y);
                        defPnt->at(3) = lcl::ldouble(dim->getDefinitionPoint().z);
                        entity->push_back(lcl::list(defPnt));

                        lclValueVec *midPnt = new lclValueVec(4);
                        midPnt->at(0) = lcl::integer(11);
                        midPnt->at(1) = lcl::ldouble(dim->getMiddleOfText().x);
                        midPnt->at(2) = lcl::ldouble(dim->getMiddleOfText().y);
                        midPnt->at(3) = lcl::ldouble(dim->getMiddleOfText().z);
                        entity->push_back(lcl::list(midPnt));

                        lclValueVec *text = new lclValueVec(3);
                        text->at(0) = lcl::integer(1);
                        text->at(1) = lcl::symbol(".");
                        text->at(2) = lcl::string(dim->getText().toStdString());
                        entity->push_back(lcl::list(text));

                        int attachmentPoint=1;
                        if (dim->getHAlign()==RS_MTextData::HALeft) {
                            attachmentPoint=1;
                        } else if (dim->getHAlign()==RS_MTextData::HACenter) {
                            attachmentPoint=2;
                        } else if (dim->getHAlign()==RS_MTextData::HARight) {
                            attachmentPoint=3;
                        }
                        if (dim->getVAlign()==RS_MTextData::VATop) {
                            attachmentPoint+=0;
                        } else if (dim->getVAlign()==RS_MTextData::VAMiddle) {
                            attachmentPoint+=3;
                        } else if (dim->getVAlign()==RS_MTextData::VABottom) {
                            attachmentPoint+=6;
                        }

                        lclValueVec *attachPnt = new lclValueVec(3);
                        attachPnt->at(0) = lcl::integer(71);
                        attachPnt->at(1) = lcl::symbol(".");
                        attachPnt->at(2) = lcl::integer(attachmentPoint);
                        entity->push_back(lcl::list(attachPnt));

                        lclValueVec *lineSpacingStyle = new lclValueVec(3);
                        lineSpacingStyle->at(0) = lcl::integer(72);
                        lineSpacingStyle->at(1) = lcl::symbol(".");
                        lineSpacingStyle->at(2) = lcl::integer(dim->getLineSpacingStyle() == RS_MTextData::MTextLineSpacingStyle::AtLeast ? 1 : 2);
                        entity->push_back(lcl::list(lineSpacingStyle));

                        lclValueVec *linespace = new lclValueVec(3);
                        linespace->at(0) = lcl::integer(41);
                        linespace->at(1) = lcl::symbol(".");
                        linespace->at(2) = lcl::ldouble(dim->getLineSpacingFactor());
                        entity->push_back(lcl::list(linespace));

                        lclValueVec *FixedLength = new lclValueVec(3);
                        FixedLength->at(0) = lcl::integer(42);
                        FixedLength->at(1) = lcl::symbol(".");
                        FixedLength->at(2) = lcl::ldouble(dim->getFixedLength());
                        entity->push_back(lcl::list(FixedLength));

                        lclValueVec *style = new lclValueVec(3);
                        style->at(0) = lcl::integer(3);
                        style->at(1) = lcl::symbol(".");
                        style->at(2) = lcl::string(dim->getData().style.toStdString());
                        entity->push_back(lcl::list(style));

                        lclValueVec *dimType = new lclValueVec(3);
                        dimType->at(0) = lcl::integer(70);
                        dimType->at(1) = lcl::symbol(".");

                        switch (e->rtti())
                        {
                        case RS2::EntityDimAligned:
                        {
                            RS_DimAligned* da = (RS_DimAligned*)e;
                            dimType->at(2) = lcl::integer(1+32);
                            entity->insert(entity->end()-6, lcl::list(dimType));

                            lclValueVec *extPnt1 = new lclValueVec(4);
                            extPnt1->at(0) = lcl::integer(13);
                            extPnt1->at(1) = lcl::ldouble(da->getExtensionPoint1().x);
                            extPnt1->at(2) = lcl::ldouble(da->getExtensionPoint1().y);
                            extPnt1->at(3) = lcl::ldouble(da->getExtensionPoint1().z);
                            entity->push_back(lcl::list(extPnt1));

                            lclValueVec *extPnt2 = new lclValueVec(4);
                            extPnt2->at(0) = lcl::integer(14);
                            extPnt2->at(1) = lcl::ldouble(da->getExtensionPoint2().x);
                            extPnt2->at(2) = lcl::ldouble(da->getExtensionPoint2().y);
                            extPnt2->at(3) = lcl::ldouble(da->getExtensionPoint2().z);
                            entity->push_back(lcl::list(extPnt2));

                            lclValueVec *acdbL2 = new lclValueVec(3);
                            acdbL2->at(0) = lcl::integer(100);
                            acdbL2->at(1) = lcl::symbol(".");
                            acdbL2->at(2) = lcl::string("AcDbAlignedDimension");
                            entity->push_back(lcl::list(acdbL2));
                            break;
                        }
                        case RS2::EntityDimDiametric:
                        {
                            RS_DimDiametric* dr = (RS_DimDiametric*)e;
                            dimType->at(2) = lcl::integer(3+32);
                            entity->insert(entity->end()-6, lcl::list(dimType));

                            lclValueVec *leader = new lclValueVec(3);
                            leader->at(0) = lcl::integer(40);
                            leader->at(1) = lcl::symbol(".");
                            leader->at(2) = lcl::ldouble(dr->getLeader());
                            entity->push_back(lcl::list(leader));

                            lclValueVec *defPnt = new lclValueVec(4);
                            defPnt->at(0) = lcl::integer(15);
                            defPnt->at(1) = lcl::ldouble(dim->getDefinitionPoint().x);
                            defPnt->at(2) = lcl::ldouble(dim->getDefinitionPoint().y);
                            defPnt->at(3) = lcl::ldouble(dim->getDefinitionPoint().z);
                            entity->push_back(lcl::list(defPnt));

                            lclValueVec *acdbL2 = new lclValueVec(3);
                            acdbL2->at(0) = lcl::integer(100);
                            acdbL2->at(1) = lcl::symbol(".");
                            acdbL2->at(2) = lcl::string("AcDbDiametricDimension");
                            entity->push_back(lcl::list(acdbL2));
                            break;
                        }
                        case RS2::EntityDimRadial:
                        {
                            RS_DimRadial* dr = (RS_DimRadial*)e;
                            dimType->at(2) = lcl::integer(4+32);
                            entity->insert(entity->end()-6, lcl::list(dimType));

                            lclValueVec *defPnt = new lclValueVec(4);
                            defPnt->at(0) = lcl::integer(15);
                            defPnt->at(1) = lcl::ldouble(dim->getDefinitionPoint().x);
                            defPnt->at(2) = lcl::ldouble(dim->getDefinitionPoint().y);
                            defPnt->at(3) = lcl::ldouble(dim->getDefinitionPoint().z);
                            entity->push_back(lcl::list(defPnt));

                            lclValueVec *leader = new lclValueVec(3);
                            leader->at(0) = lcl::integer(40);
                            leader->at(1) = lcl::symbol(".");
                            leader->at(2) = lcl::ldouble(dr->getLeader());
                            entity->push_back(lcl::list(leader));

                            lclValueVec *acdbL2 = new lclValueVec(3);
                            acdbL2->at(0) = lcl::integer(100);
                            acdbL2->at(1) = lcl::symbol(".");
                            acdbL2->at(2) = lcl::string("AcDbRadialDimension");
                            entity->push_back(lcl::list(acdbL2));
                            break;
                        }
                        case RS2::EntityDimAngular:
                        {
                            RS_DimAngular* da = static_cast<RS_DimAngular*>(e);
                            if (da->getDefinitionPoint3() == da->getData().definitionPoint) {
                                //DimAngular3p
                                dimType->at(2) = lcl::integer(5+32);
                                entity->insert(entity->end()-6, lcl::list(dimType));

                                lclValueVec *defPnt1 = new lclValueVec(4);
                                defPnt1->at(0) = lcl::integer(13);
                                defPnt1->at(1) = lcl::ldouble(da->getDefinitionPoint().x);
                                defPnt1->at(2) = lcl::ldouble(da->getDefinitionPoint().y);
                                defPnt1->at(3) = lcl::ldouble(da->getDefinitionPoint().z);
                                entity->push_back(lcl::list(defPnt1));

                                lclValueVec *defPnt2 = new lclValueVec(4);
                                defPnt2->at(0) = lcl::integer(14);
                                defPnt2->at(1) = lcl::ldouble(da->getDefinitionPoint().x);
                                defPnt2->at(2) = lcl::ldouble(da->getDefinitionPoint().y);
                                defPnt2->at(3) = lcl::ldouble(da->getDefinitionPoint().z);
                                entity->push_back(lcl::list(defPnt2));

                                lclValueVec *defPnt3 = new lclValueVec(4);
                                defPnt3->at(0) = lcl::integer(15);
                                defPnt3->at(1) = lcl::ldouble(da->getDefinitionPoint().x);
                                defPnt3->at(2) = lcl::ldouble(da->getDefinitionPoint().y);
                                defPnt3->at(3) = lcl::ldouble(da->getDefinitionPoint().z);
                                entity->push_back(lcl::list(defPnt3));

                            }
                            else
                            {
                                //DimAngular
                                dimType->at(2) = lcl::integer(2+32);
                                entity->insert(entity->end()-6, lcl::list(dimType));

                                lclValueVec *defPnt1 = new lclValueVec(4);
                                defPnt1->at(0) = lcl::integer(13);
                                defPnt1->at(1) = lcl::ldouble(da->getDefinitionPoint1().x);
                                defPnt1->at(2) = lcl::ldouble(da->getDefinitionPoint1().y);
                                defPnt1->at(3) = lcl::ldouble(da->getDefinitionPoint1().z);
                                entity->push_back(lcl::list(defPnt1));

                                lclValueVec *defPnt2 = new lclValueVec(4);
                                defPnt2->at(0) = lcl::integer(14);
                                defPnt2->at(1) = lcl::ldouble(da->getDefinitionPoint2().x);
                                defPnt2->at(2) = lcl::ldouble(da->getDefinitionPoint2().y);
                                defPnt2->at(3) = lcl::ldouble(da->getDefinitionPoint2().z);
                                entity->push_back(lcl::list(defPnt2));

                                lclValueVec *defPnt3 = new lclValueVec(4);
                                defPnt3->at(0) = lcl::integer(15);
                                defPnt3->at(1) = lcl::ldouble(da->getDefinitionPoint3().x);
                                defPnt3->at(2) = lcl::ldouble(da->getDefinitionPoint3().y);
                                defPnt3->at(3) = lcl::ldouble(da->getDefinitionPoint3().z);
                                entity->push_back(lcl::list(defPnt3));

                                lclValueVec *defPnt4 = new lclValueVec(4);
                                defPnt4->at(0) = lcl::integer(16);
                                defPnt4->at(1) = lcl::ldouble(da->getDefinitionPoint4().x);
                                defPnt4->at(2) = lcl::ldouble(da->getDefinitionPoint4().y);
                                defPnt4->at(3) = lcl::ldouble(da->getDefinitionPoint4().z);
                                entity->push_back(lcl::list(defPnt4));
                            }

                            lclValueVec *acdbL2 = new lclValueVec(3);
                            acdbL2->at(0) = lcl::integer(100);
                            acdbL2->at(1) = lcl::symbol(".");
                            acdbL2->at(2) = lcl::string("AcDb3PointAngularDimension");
                            entity->push_back(lcl::list(acdbL2));
                            break;
                        }
                        default:
                        { //default to DimLinear
                            RS_DimLinear* dl = (RS_DimLinear*)e;
                            dimType->at(2) = lcl::integer(0+32);
                            entity->insert(entity->end()-6, lcl::list(dimType));

                            lclValueVec *acdbL2 = new lclValueVec(3);
                            acdbL2->at(0) = lcl::integer(100);
                            acdbL2->at(1) = lcl::symbol(".");
                            acdbL2->at(2) = lcl::string("AcDbAlignedDimension");
                            entity->push_back(lcl::list(acdbL2));

                            lclValueVec *extPnt1 = new lclValueVec(4);
                            extPnt1->at(0) = lcl::integer(13);
                            extPnt1->at(1) = lcl::ldouble(dl->getExtensionPoint1().x);
                            extPnt1->at(2) = lcl::ldouble(dl->getExtensionPoint1().y);
                            extPnt1->at(3) = lcl::ldouble(dl->getExtensionPoint1().z);
                            entity->push_back(lcl::list(extPnt1));

                            lclValueVec *extPnt2 = new lclValueVec(4);
                            extPnt2->at(0) = lcl::integer(14);
                            extPnt2->at(1) = lcl::ldouble(dl->getExtensionPoint2().x);
                            extPnt2->at(2) = lcl::ldouble(dl->getExtensionPoint2().y);
                            extPnt2->at(3) = lcl::ldouble(dl->getExtensionPoint2().z);
                            entity->push_back(lcl::list(extPnt2));

                            lclValueVec *angle = new lclValueVec(3);
                            angle->at(0) = lcl::integer(50);
                            angle->at(1) = lcl::symbol(".");
                            angle->at(2) = lcl::ldouble(dl->getAngle()*180/M_PI);
                            entity->push_back(lcl::list(angle));

                            lclValueVec *oblique = new lclValueVec(3);
                            oblique->at(0) = lcl::integer(52);
                            oblique->at(1) = lcl::symbol(".");
                            oblique->at(2) = lcl::ldouble(dl->getOblique());
                            entity->push_back(lcl::list(oblique));

                            lclValueVec *acdbL3 = new lclValueVec(3);
                            acdbL3->at(0) = lcl::integer(100);
                            acdbL3->at(1) = lcl::symbol(".");
                            acdbL3->at(2) = lcl::string("AcDbRotatedDimension");
                            entity->push_back(lcl::list(acdbL3));
                            break;
                        }
                        }
                        break;
                    }
                    case RS2::EntityDimLeader:
                    // not implemented yet
                        delete ename;
                        delete handle;
                        delete acdb;
                        delete mspace;
                        delete layoutTabName;
                        delete layer;
                        delete extrDir;

                        return lcl::nilValue();
                        break;

                    case RS2::EntityHatch:
                    {
                        RS_Hatch* h = (RS_Hatch*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("HATCH");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbHatch");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(0.0);
                        pnt->at(2) = lcl::ldouble(0.0);
                        pnt->at(3) = lcl::ldouble(0.0);
                        entity->push_back(lcl::list(pnt));

                        entity->push_back(lcl::list(extrDir));

                        lclValueVec *pattern = new lclValueVec(3);
                        pattern->at(0) = lcl::integer(2);
                        pattern->at(1) = lcl::symbol(".");
                        pattern->at(2) = lcl::string(qUtf8Printable(h->getPattern()));
                        entity->push_back(lcl::list(pattern));

                        if(!h->isSolid())
                        {
                            lclValueVec *angle = new lclValueVec(3);
                            angle->at(0) = lcl::integer(52);
                            angle->at(1) = lcl::symbol(".");
                            angle->at(2) = lcl::ldouble(h->getAngle());
                            entity->push_back(lcl::list(angle));

                            lclValueVec *scale = new lclValueVec(3);
                            scale->at(0) = lcl::integer(41);
                            scale->at(1) = lcl::symbol(".");
                            scale->at(2) = lcl::ldouble(h->getScale());
                            entity->push_back(lcl::list(scale));
                        }

                        lclValueVec *solidFill = new lclValueVec(3);
                        solidFill->at(0) = lcl::integer(70);
                        solidFill->at(1) = lcl::symbol(".");
                        solidFill->at(2) = lcl::integer(static_cast<int>(h->isSolid()));
                        entity->push_back(lcl::list(solidFill));
                        break;
                    }

                    case RS2::EntityImage:
                    {
                        RS_Image* img = (RS_Image*)e;

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("IMAGE");
                        entity->insert(entity->begin(), lcl::list(name));
                        entity->insert(entity->begin(), lcl::list(ename));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbRasterImage");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(img->getInsertionPoint().x);
                        pnt->at(2) = lcl::ldouble(img->getInsertionPoint().y);
                        pnt->at(3) = lcl::ldouble(img->getInsertionPoint().z);
                        entity->push_back(lcl::list(pnt));

                        lclValueVec *uVector = new lclValueVec(4);
                        uVector->at(0) = lcl::integer(11);
                        uVector->at(1) = lcl::ldouble(img->getUVector().x);
                        uVector->at(2) = lcl::ldouble(img->getUVector().y);
                        uVector->at(3) = lcl::ldouble(img->getUVector().z);
                        entity->push_back(lcl::list(uVector));

                        lclValueVec *vVector = new lclValueVec(4);
                        vVector->at(0) = lcl::integer(12);
                        vVector->at(1) = lcl::ldouble(img->getVVector().x);
                        vVector->at(2) = lcl::ldouble(img->getVVector().y);
                        vVector->at(3) = lcl::ldouble(img->getVVector().z);
                        entity->push_back(lcl::list(vVector));

                        lclValueVec *size = new lclValueVec(3);
                        size->at(0) = lcl::integer(13);
                        size->at(1) = lcl::ldouble(double(img->getWidth()));
                        size->at(2) = lcl::ldouble(double(img->getHeight()));
                        entity->push_back(lcl::list(size));

                        lclValueVec *file = new lclValueVec(3);
                        file->at(0) = lcl::integer(340);
                        file->at(1) = lcl::symbol(".");
                        file->at(2) = lcl::string(qUtf8Printable(img->getFile()));
                        entity->push_back(lcl::list(file));

                        // missing value Image display properties ad dummy
                        lclValueVec *prop = new lclValueVec(3);
                        prop->at(0) = lcl::integer(70);
                        prop->at(1) = lcl::symbol(".");
                        prop->at(2) = lcl::integer(1);
                        entity->push_back(lcl::list(prop));

                        // missing value Clipping state ad dummy
                        lclValueVec *clippingState = new lclValueVec(3);
                        clippingState->at(0) = lcl::integer(280);
                        clippingState->at(1) = lcl::symbol(".");
                        clippingState->at(2) = lcl::integer(0);
                        entity->push_back(lcl::list(clippingState));

                        lclValueVec *brightness = new lclValueVec(3);
                        brightness->at(0) = lcl::integer(281);
                        brightness->at(1) = lcl::symbol(".");
                        brightness->at(2) = lcl::integer(img->getBrightness());
                        entity->push_back(lcl::list(brightness));

                        lclValueVec *contrast = new lclValueVec(3);
                        contrast->at(0) = lcl::integer(282);
                        contrast->at(1) = lcl::symbol(".");
                        contrast->at(2) = lcl::integer(img->getContrast());
                        entity->push_back(lcl::list(contrast));

                        lclValueVec *fade = new lclValueVec(3);
                        fade->at(0) = lcl::integer(283);
                        fade->at(1) = lcl::symbol(".");
                        fade->at(2) = lcl::integer(img->getFade());
                        entity->push_back(lcl::list(fade));

                        // we got only rect boundary
                        lclValueVec *boundary = new lclValueVec(3);
                        boundary->at(0) = lcl::integer(71);
                        boundary->at(1) = lcl::symbol(".");
                        boundary->at(2) = lcl::integer(1);
                        entity->push_back(lcl::list(boundary));

                        for (auto &v : img->getCorners())
                        {
                            lclValueVec *cor = new lclValueVec(4);
                            cor->at(0) = lcl::integer(14);
                            cor->at(1) = lcl::ldouble(v.x);
                            cor->at(2) = lcl::ldouble(v.y);
                            cor->at(3) = lcl::ldouble(v.z);
                            entity->push_back(lcl::list(cor));
                        }
                        entity->push_back(lcl::list(extrDir));
                        break;
                    }

                    default:
                    {
                        delete ename;
                        delete handle;
                        delete acdb;
                        delete mspace;
                        delete layoutTabName;
                        delete layer;
                        delete extrDir;

                        return lcl::nilValue();
                        break;
                    }
                }
                return lcl::list(entity);
            }
#if 0
        }
#endif
    }
    return lcl::nilValue();
}

static lclValuePtr getvar(const std::string &sysvar)
{
    int int_result;
    double v1_result;
    double v2_result;
    double v3_result;
    std::string str_result;

    RS_ScriptingApi::SysVarResult val = RS_SCRIPTINGAPI->getvar(sysvar.c_str(), int_result, v1_result, v2_result, v3_result, str_result);

    switch (val) {
    case RS_ScriptingApi::Int:
        return lcl::integer(int_result);
    case RS_ScriptingApi::Double:
        return lcl::ldouble(v1_result);
    case RS_ScriptingApi::Vector2D:
    {
        lclValueVec* result = new lclValueVec(2);
        result->at(0) = lcl::ldouble(v1_result);
        result->at(1) = lcl::ldouble(v2_result);
        return lcl::list(result);
    }
    case RS_ScriptingApi::Vector3D:
    {
        lclValueVec* result = new lclValueVec(3);
        result->at(0) = lcl::ldouble(v1_result);
        result->at(1) = lcl::ldouble(v2_result);
        result->at(2) = lcl::ldouble(v3_result);
        return lcl::list(result);
    }
    case RS_ScriptingApi::String:
        return lcl::string(str_result);
    default:
        break;
    }

    return lcl::nilValue();
}

void installCore(lclEnvPtr env) {
    for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it) {
        lclBuiltIn* handler = *it;
        env->set(handler->name(), handler);
    }
}

static String printValues(lclValueIter begin, lclValueIter end,
                          const String& sep, bool readably)
{
    String out;

    if (begin != end) {
        out += (*begin)->print(readably);
        ++begin;
    }

    for ( ; begin != end; ++begin) {
        out += sep;
        out += (*begin)->print(readably);
    }

    return out;
}

static int countValues(lclValueIter begin, lclValueIter end)
{
    int result = 0;

    if (begin != end) {
        result += (*begin)->print(true).length() -2;
        ++begin;
    }

    for ( ; begin != end; ++begin) {
        result += (*begin)->print(true).length() -2;
    }

    return result;
}

bool getApiData(lclValueVec* items, RS_ScriptingApiData &apiData)
{
    const int length = items->size();

    for (int i = 0; i < length; i++) {
        if (items->at(i)->type() == LCLTYPE::LIST ||
                items->at(i)->type() == LCLTYPE::VEC)
        {
            const lclSequence* list = VALUE_CAST(lclSequence, items->at(i));
            const lclInteger* gc = VALUE_CAST(lclInteger, list->item(0));

            switch (gc->value())
            {
            case -1:
            {
                if (!list->isDotted())
                {
                    return false;
                }
                const lclEname *en = VALUE_CAST(lclEname, list->item(2));
                apiData.id.push_back({ en->value() });
            }
                break;
            case 0:
            {
                if (!list->isDotted())
                {
                    return false;
                }
                const lclString *n = VALUE_CAST(lclString, list->item(2));
                apiData.etype = n->value().c_str();
            }
                break;
            case 1:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclString *t = VALUE_CAST(lclString, list->item(2));
                apiData.text = t->value().c_str();
            }
                break;
            case 2:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclString *l = VALUE_CAST(lclString, list->item(2));
                apiData.block = l->value().c_str();
            }
                break;
            case 6:
            {
                if (!list->isDotted())
                {
                    return lcl::nilValue();
                }

                const lclString *ltype = VALUE_CAST(lclString, list->item(2));
                apiData.linetype = ltype->value().c_str();
                apiData.pen.setLineType(RS_FilterDXFRW::nameToLineType(ltype->value().c_str()));
            }
                break;
            case 7:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclString *s = VALUE_CAST(lclString, list->item(2));
                apiData.style = s->value().c_str();
            }
                break;
            case 8:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclString *l = VALUE_CAST(lclString, list->item(2));
                apiData.layer = l->value().c_str();
            }
                break;
            case 10:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_10.push_back({ pnt });
            }
                break;
            case 11:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_11.push_back({ pnt });
            }
                break;
            case 12:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_12.push_back({ pnt });
            }
                break;
            case 13:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_13.push_back({ pnt });
            }
                break;
            case 14:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_14.push_back({ pnt });
            }
                break;
            case 15:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_15.push_back({ pnt });
            }
                break;
            case 16:
            {
                if (list->count() < 3)
                {
                    return false;
                }

                RS_Vector pnt;

                if (list->item(1)->type() == LCLTYPE::INT)
                {
                    const lclInteger *x = VALUE_CAST(lclInteger, list->item(1));
                    pnt.x = double(x->value());
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    pnt.x = x->value();
                }
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *y = VALUE_CAST(lclInteger, list->item(2));
                    pnt.y = double(y->value());
                }
                else
                {
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    pnt.y = y->value();
                }

                if (list->count() > 3)
                {
                    if (list->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *z = VALUE_CAST(lclInteger, list->item(3));
                        pnt.z = double(z->value());
                    }
                    else
                    {
                        const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                        pnt.z = z->value();
                    }
                }

                apiData.gc_16.push_back({ pnt });
            }
                break;
            case 40:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *r1 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_40.push_back({ double(r1->value()) });
                }
                else
                {
                    const lclDouble *r1 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_40.push_back({ r1->value() });
                }
            }
                break;
            case 41:
            {
                if (!list->isDotted())
                {
                   return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *r2 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_41.push_back({ double(r2->value()) });
                }
                else
                {
                    const lclDouble *r2 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_41.push_back({ r2->value() });
                }
            }
                break;
            case 42:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *r3 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_42.push_back({ double(r3->value()) });
                }
                else
                {
                    const lclDouble *r3 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_42.push_back({ r3->value() });
                }
            }
                break;
            case 43:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *r3 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_43.push_back({ double(r3->value()) });
                }
                else
                {
                    const lclDouble *r3 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_43.push_back({ r3->value() });
                }
            }
                break;
            case 44:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclDouble *sc1 = VALUE_CAST(lclDouble, list->item(2));
                apiData.gc_44.push_back({ sc1->value() });
            }
                break;
            case 45:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclDouble *sc2 = VALUE_CAST(lclDouble, list->item(2));
                apiData.gc_45.push_back({ sc2->value() });
            }
                break;
            case 48:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                int width = 0;
                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *lwidth = VALUE_CAST(lclInteger, list->item(2));
                    if (lwidth->value() >= 0)
                    {
                        width = lwidth->value() * 100;
                    }
                    else
                    {
                        width = lwidth->value();
                    }
                }
                else
                {
                    const lclDouble *lwidth = VALUE_CAST(lclDouble, list->item(2));
                    if (lwidth->value() >= 0)
                    {
                        width = static_cast<int>(lwidth->value()) * 100;
                    }
                    else
                    {
                        width = static_cast<int>(lwidth->value());
                    }
                }

                apiData.gc_lineWidth.push_back(width);
                apiData.pen.setWidth(RS2::intToLineWidth(width));
            }
                break;
            case 50:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *a1 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_50.push_back({ double(a1->value()) });
                }
                else
                {
                    const lclDouble *a1 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_50.push_back({ a1->value() });
                }
            }
                break;
            case 51:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *a2 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_51.push_back({ double(a2->value()) });
                }
                else
                {
                    const lclDouble *a2 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_51.push_back({ a2->value() });
                }
            }
                break;
            case 52:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *a2 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_52.push_back({ double(a2->value()) });
                }
                else
                {
                    const lclDouble *a2 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_52.push_back({ a2->value() });
                }
            }
                break;
            case 53:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                if (list->item(2)->type() == LCLTYPE::INT)
                {
                    const lclInteger *a2 = VALUE_CAST(lclInteger, list->item(2));
                    apiData.gc_53.push_back({ double(a2->value()) });
                }
                else
                {
                    const lclDouble *a2 = VALUE_CAST(lclDouble, list->item(2));
                    apiData.gc_53.push_back({ a2->value() });
                }
            }
                break;
            case 62:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *color = VALUE_CAST(lclInteger, list->item(2));
                apiData.pen.setColor(RS_FilterDXFRW::numberToColor(color->value()));
                apiData.gc_62.push_back({ color->value() });
            }
                break;
            case 70:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f0 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_70.push_back({f0->value() });
            }
                break;
            case 71:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f1 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_71.push_back({f1->value() });
            }
                break;
            case 72:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f2 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_72.push_back({f2->value() });
            }
                break;
            case 73:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f3 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_73.push_back({f3->value() });
            }
                break;
            case 100:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclString *n = VALUE_CAST(lclString, list->item(2));
                apiData.gc_100.push_back({ n->value().c_str() });
            }
                break;
            case 281:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f3 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_281.push_back({f3->value() });
            }
                break;
            case 282:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f3 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_282.push_back({f3->value() });
            }
                break;
            case 283:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *f3 = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_283.push_back({f3->value() });
            }
                break;
            case 402:
            {
                if (!list->isDotted())
                {
                    return false;
                }

                const lclInteger *c = VALUE_CAST(lclInteger, list->item(2));
                apiData.gc_402.push_back({c->value() });
            }
                break;
            default:
                break;
            }
        }
    }
    return true;
}

#endif // DEVELOPER
