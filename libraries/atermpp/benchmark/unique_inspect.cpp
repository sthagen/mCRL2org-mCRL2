// Author(s): Maurice Laveaux
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "benchmark_shared.h"

#include <queue>

using namespace atermpp;

int main(int argc, char* argv[])
{
  detail::g_term_pool().enable_garbage_collection(false);
  std::size_t number_of_threads = 1;

  // Accept one argument for the number of threads.
  if (argc > 1)
  {
    number_of_threads = static_cast<std::size_t>(std::stoi(argv[1]));
  }

  std::size_t size = 20;
  std::size_t iterations = 1000;

  std::vector<aterm_appl> terms;
  for (std::size_t id = 0; id < number_of_threads; ++id)
  {
    terms.push_back(create_nested_function<2>("f", std::to_string(id), size));
  }

  // Define a function that repeatedly creates nested function applications.
  auto nested_function = [&](int id) -> void
    {
      std::size_t n = 0;
      std::queue<const aterm_appl*> Q;
      for (std::size_t i = 0; i < iterations / number_of_threads; ++i)
      {
        n = 0;

        // A simple breadth first search to count the number of elements.
        Q.push(&terms[id]);

        while (!Q.empty())
        {
          const aterm_appl& t = *Q.front();
          Q.pop();

          for (const aterm& u : t)
          {
            Q.push(reinterpret_cast<const aterm_appl*>(&u));
            n++;
          }
        }
      }

      std::cerr << "Term has " << n << " elements" << std::endl;
    };

  benchmark_threads(number_of_threads, nested_function);
  return 0;
}
