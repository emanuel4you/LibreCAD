%module librecad;

%rename(__aref__)             *::operator[];
%rename(__lshift__)           *::operator<<;
%rename(__rshift__)           *::operator>>;

%{
#include <string>
#include "rs_vector.h"
#include "rs_python_core.h"
#include "rs_python_gui.h"
#include "rs_python_dcl.h"
#include "rs_python_lisp.h"
%}

%include <std_string.i>
%include "lib/engine/rs_vector.h"
%include "lib/scripting/python/rs_python_core.h"
%include "lib/scripting/python/rs_python_gui.h"
%include "lib/scripting/python/rs_python_dcl.h"
%include "lib/scripting/python/rs_python_lisp.h"
