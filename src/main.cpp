// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2010, Jonathan Turner (jonathan@emptycrate.com)
// and Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#include <iostream>

#include <list>

#define _CRT_SECURE_NO_WARNINGS

#ifdef READLINE_AVAILABLE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <chaiscript/chaiscript.hpp>

void print_help() {
  std::cout << "ChaiScript evaluator.  To evaluate an expression, type it and press <enter>." << std::endl;
  std::cout << "Additionally, you can inspect the runtime system using:" << std::endl;
  std::cout << "  dump_system() - outputs all functions registered to the system" << std::endl;
  std::cout << "  dump_object(x) - dumps information about the given symbol" << std::endl;
}


bool throws_exception(const chaiscript::Proxy_Function &f)
{
  try {
    chaiscript::functor<void ()>(f)();
  } catch (...) {
    return true;
  }

  return false;
}


std::string get_next_command() {
#ifdef READLINE_AVAILABLE
  char *input_raw;
  input_raw = readline("eval> ");
  add_history(input_raw);
  return std::string(input_raw);
#else
  std::string retval;
  std::cout << "eval> ";
  std::getline(std::cin, retval);
  return retval;
#endif
}

// We have to wrap exit with our own because Clang has a hard time with
// function pointers to functions with special attributes (system exit being marked NORETURN)
void myexit(int return_val) {
  exit(return_val);
}

int main(int argc, char *argv[]) {
  std::string input;

  std::vector<std::string> usepaths;
  std::vector<std::string> modulepaths;


  // Disable deprecation warning for getenv call.
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

  const char *usepath = getenv("CHAI_USE_PATH");
  const char *modulepath = getenv("CHAI_MODULE_PATH");

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

  usepaths.push_back("");

  if (usepath)
  {
    usepaths.push_back(usepath);
  }

  modulepaths.push_back("");

  if (modulepath)
  {
    modulepaths.push_back(modulepath);
  }


  chaiscript::ChaiScript chai(modulepaths,usepaths);

  chai.add(chaiscript::fun(&myexit), "exit");
  chai.add(chaiscript::fun(&throws_exception), "throws_exception");

  if (argc < 2) {
#ifdef READLINE_AVAILABLE
    using_history();
#endif
    input = get_next_command();
    while (input != "quit") {

      chaiscript::Boxed_Value val;

      if (input == "help") {
        print_help();
      }
      else {
        try {
          //First, we evaluate it
          val = chai.eval(input);

          //Then, we try to print the result of the evaluation to the user
          if (!val.get_type_info().bare_equal(chaiscript::user_type<void>())) {
            try {
              chaiscript::dispatch(chai.get_eval_engine().get_function("print"), chaiscript::Param_List_Builder() << val);
            }
            catch (...) {
              //If we can't, do nothing
            }
          }
        }
        catch (chaiscript::Eval_Error &ee) {
          std::cout << ee.what();
          if (ee.call_stack.size() > 0) {
            std::cout << "during evaluation at (" << ee.call_stack[0]->start.line << ", " << ee.call_stack[0]->start.column << ")";
          }
          std::cout << std::endl;
        }
        catch (std::exception &e) {
          std::cout << e.what();
          std::cout << std::endl;
        }
      }

      input = get_next_command();
    }
  }
  else {
    for (int i = 1; i < argc; ++i) {
      try {
        chaiscript::Boxed_Value val = chai.eval_file(argv[i]);
      }
      catch (chaiscript::Eval_Error &ee) {
        std::cout << ee.what();
        if (ee.call_stack.size() > 0) {
          std::cout << "during evaluation at (" << *(ee.call_stack[0]->filename) << " " << ee.call_stack[0]->start.line << ", " << ee.call_stack[0]->start.column << ")";
          for (unsigned int j = 1; j < ee.call_stack.size(); ++j) {
            std::cout << std::endl;
            std::cout << "  from " << ee.call_stack[j]->filename << " (" << ee.call_stack[j]->start.line << ", " << ee.call_stack[j]->start.column << ")";
          }
        }
        std::cout << std::endl;
        return EXIT_FAILURE;
      }
      catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

