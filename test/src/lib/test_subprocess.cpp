/**
 * @file test_subprocess.cpp
 * @brief Unit tests for subprocess.hpp process spawning utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#include "../../../src/lib/env.hpp"
#include "../../../src/lib/subprocess.hpp"

using namespace ns_subprocess;

TEST_CASE("ns_subprocess::to_carray converts vector to C array")
{
  std::vector<std::string> vec = {"arg1", "arg2", "arg3"};

  auto arr = to_carray(vec);

  REQUIRE_NE(arr, nullptr);
  CHECK_EQ(std::string(arr[0]), "arg1");
  CHECK_EQ(std::string(arr[1]), "arg2");
  CHECK_EQ(std::string(arr[2]), "arg3");
  CHECK_EQ(arr[3], nullptr); // Null-terminated
}

TEST_CASE("ns_subprocess::to_carray handles empty vector")
{
  std::vector<std::string> vec;

  auto arr = to_carray(vec);

  REQUIRE_NE(arr, nullptr);
  CHECK_EQ(arr[0], nullptr);
}

TEST_CASE("ns_subprocess::to_carray handles single element")
{
  std::vector<std::string> vec = {"single"};

  auto arr = to_carray(vec);

  REQUIRE_NE(arr, nullptr);
  CHECK_EQ(std::string(arr[0]), "single");
  CHECK_EQ(arr[1], nullptr);
}

TEST_CASE("Subprocess constructor accepts program path")
{
  Subprocess proc("/bin/echo");

  // Constructor should not throw
  CHECK(true);
}

TEST_CASE("Subprocess::with_args adds arguments")
{
  Subprocess proc("/bin/echo");

  auto& result = proc.with_args("hello", "world");

  // Should return reference for chaining
  CHECK_EQ(&result, &proc);
}

TEST_CASE("Subprocess::env_clear removes environment variables")
{
  // Set a test variable in current environment
  setenv("TEST_ENV_CLEAR", "should_be_cleared", 1);
  REQUIRE_NE(getenv("TEST_ENV_CLEAR"), nullptr);

  // Find env binary to list all environment variables
  auto env_path = ns_env::search_path("env");
  REQUIRE(env_path.has_value());

  // Spawn env with cleared environment
  std::ostringstream output;
  Subprocess proc(env_path.value());
  auto child = proc
    .env_clear()
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output, std::cerr)
    .spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code.has_value());
  CHECK_EQ(*exit_code, 0);

  // Output should be empty or minimal (no inherited environment)
  std::string result_str = output.str();
  // Should not contain our test variable
  CHECK_EQ(result_str.find("TEST_ENV_CLEAR"), std::string::npos);

  // Clean up
  unsetenv("TEST_ENV_CLEAR");
}

TEST_CASE("Subprocess::with_var sets environment variable")
{
  // Find printenv binary
  auto path_bin = ns_env::search_path("sh");
  REQUIRE(path_bin.has_value());

  // Spawn printenv to check if the variable is set
  std::ostringstream output;
  auto child = ns_subprocess::Subprocess(path_bin.value())
    .with_args("-c", "echo $TEST_VAR_SET")
    .with_var("TEST_VAR_SET", "test_value_123")
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output, output)
    .spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code.has_value());
  CHECK_EQ(*exit_code, 0);

  // Output should contain the value we set
  std::string result = output.str();
  CHECK_EQ(result, "test_value_123\n");
}

TEST_CASE("Subprocess::with_env adds environment entries")
{
  // Find printenv binary
  auto printenv_path = ns_env::search_path("printenv");
  REQUIRE(printenv_path.has_value());

  using namespace std::string_literals;

  // Test first variable
  std::ostringstream output1;
  Subprocess proc1(printenv_path.value());
  auto child1 = proc1.with_args("VAR1")
    .with_env("VAR1=value1"s, "VAR2=value2"s)
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output1, std::cerr)
    .spawn();

  REQUIRE(child1->get_pid().has_value());
  auto exit_code1 = child1->wait();

  REQUIRE(exit_code1.has_value());
  CHECK_EQ(*exit_code1, 0);
  CHECK_EQ(output1.str(), "value1\n");

  // Test second variable
  std::ostringstream output2;
  Subprocess proc2(printenv_path.value());
  auto child2 = proc2.with_args("VAR2")
    .with_env("VAR1=value1"s, "VAR2=value2"s)
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output2, std::cerr)
    .spawn();

  REQUIRE(child2->get_pid().has_value());
  auto exit_code2 = child2->wait();

  REQUIRE(exit_code2.has_value());
  CHECK_EQ(*exit_code2, 0);
  CHECK_EQ(output2.str(), "value2\n");
}

TEST_CASE("Subprocess::rm_var removes environment variable")
{
  // First, set an environment variable in the current process
  setenv("TEST_REMOVE_VAR", "should_be_removed", 1);

  // Verify it exists in current environment
  REQUIRE_NE(getenv("TEST_REMOVE_VAR"), nullptr);

  // Find printenv binary
  auto printenv_path = ns_env::search_path("printenv");
  REQUIRE(printenv_path.has_value());

  // Spawn printenv to check if the variable exists
  std::ostringstream output;
  Subprocess proc(printenv_path.value());
  auto child = proc.with_args("TEST_REMOVE_VAR")
    .rm_var("TEST_REMOVE_VAR")
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output, std::cerr)
    .spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code.has_value());

  // printenv returns non-zero when variable doesn't exist
  CHECK_NE(*exit_code, 0);

  // Output should be empty since variable was removed
  std::string result_str = output.str();
  CHECK(result_str.empty());

  // Clean up
  unsetenv("TEST_REMOVE_VAR");
}

TEST_CASE("Subprocess::rm_var is safe on non-existent variable")
{
  // Find printenv binary
  auto printenv_path = ns_env::search_path("printenv");
  REQUIRE(printenv_path.has_value());

  // Spawn process that removes a non-existent variable (should not crash)
  std::ostringstream output;
  Subprocess proc(printenv_path.value());
  auto child = proc
    .with_args("DOES_NOT_EXIST_VAR_12345")
    .rm_var("DOES_NOT_EXIST_VAR_12345")
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output, std::cerr)
    .spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  // Should succeed (exit code is non-zero because var doesn't exist, but process runs)
  REQUIRE(exit_code.has_value());
}

TEST_CASE("Subprocess::rm_var allows chaining")
{
  // Set multiple test variables
  setenv("TEST_CHAIN_VAR1", "value1", 1);
  setenv("TEST_CHAIN_VAR2", "value2", 1);
  setenv("TEST_CHAIN_VAR3", "value3", 1);

  REQUIRE_NE(getenv("TEST_CHAIN_VAR1"), nullptr);
  REQUIRE_NE(getenv("TEST_CHAIN_VAR2"), nullptr);
  REQUIRE_NE(getenv("TEST_CHAIN_VAR3"), nullptr);

  // Find env binary
  auto env_path = ns_env::search_path("env");
  REQUIRE(env_path.has_value());

  // Remove all three variables via chaining
  std::stringstream output;
  Subprocess proc(env_path.value());
  auto child = proc
    .rm_var("TEST_CHAIN_VAR1")
    .rm_var("TEST_CHAIN_VAR2")
    .rm_var("TEST_CHAIN_VAR3")
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output, std::cerr)
    .spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();
  REQUIRE(exit_code.has_value());
  CHECK_EQ(*exit_code, 0);

  // None of the variables should appear in the output
  std::string result_str = output.str();
  CHECK_EQ(result_str.find("TEST_CHAIN_VAR1"), std::string::npos);
  CHECK_EQ(result_str.find("TEST_CHAIN_VAR2"), std::string::npos);
  CHECK_EQ(result_str.find("TEST_CHAIN_VAR3"), std::string::npos);

  // Clean up
  unsetenv("TEST_CHAIN_VAR1");
  unsetenv("TEST_CHAIN_VAR2");
  unsetenv("TEST_CHAIN_VAR3");
}

TEST_CASE("Subprocess method chaining works")
{
  Subprocess proc("/bin/echo");

  // All methods should be chainable
  auto& result = proc.with_args("test").with_stdio(Stream::Null).with_var("TEST", "value");

  CHECK_EQ(&result, &proc);
}

TEST_CASE("Subprocess::spawn executes simple command")
{
  Subprocess proc("/bin/true");

  auto child = proc.spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code);
  CHECK_EQ(*exit_code, 0);
}

TEST_CASE("Subprocess::spawn handles command that fails")
{
  Subprocess proc("/bin/false");

  auto child = proc.spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code);
  CHECK_NE(exit_code.value(), 0);
}

TEST_CASE("Subprocess::spawn with arguments")
{
  Subprocess proc("/bin/echo");

  auto child = proc.with_args("hello").with_stdio(Stream::Null).spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code);
  CHECK_EQ(*exit_code, 0);
}

TEST_CASE("Subprocess runs with different stream modes")
{
  Subprocess proc1("/bin/true");
  auto child1 = proc1.with_stdio(Stream::Inherit).spawn();
  REQUIRE(child1->get_pid().has_value());
  auto result1 = child1->wait();
  REQUIRE(result1.has_value());
  CHECK_EQ(*result1, 0);

  Subprocess proc2("/bin/true");
  auto child2 = proc2.with_stdio(Stream::Null).spawn();
  REQUIRE(child2->get_pid().has_value());
  auto result2 = child2->wait();
  REQUIRE(result2.has_value());
  CHECK_EQ(*result2, 0);
}

TEST_CASE("Subprocess executes /bin/sh command")
{
  // Find sh in PATH
  Subprocess proc("/bin/sh");

  auto child = proc.with_args("-c", "exit 0").with_stdio(Stream::Null).spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code);
  CHECK_EQ(*exit_code, 0);
}

TEST_CASE("Subprocess handles exit codes correctly")
{
  Subprocess proc("/bin/sh");

  auto child = proc.with_args("-c", "exit 42").with_stdio(Stream::Null).spawn();

  REQUIRE(child->get_pid().has_value());
  auto exit_code = child->wait();

  REQUIRE(exit_code);
  CHECK_EQ(*exit_code, 42);
}
