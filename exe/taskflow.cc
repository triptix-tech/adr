// This program demonstrates how to use tf::DataPipeline to create
// a pipeline with in-pipe data automatically managed by the Taskflow
// library.

#include <taskflow/algorithm/data_pipeline.hpp>
#include <taskflow/taskflow.hpp>

int main() {
  // dataflow => void -> int -> std::string -> float -> void
  tf::Taskflow taskflow("pipeline");
  tf::Executor executor;

  const size_t num_lines = 100;

  // create a pipeline graph
  tf::DataPipeline pl(
      num_lines,
      tf::make_data_pipe<void, int>(
          tf::PipeType::SERIAL,
          [&](tf::Pipeflow& pf) {
            if (pf.token() == 5) {
              pf.stop();
              return 0;
            } else {
              printf("___ first pipe returns %zu\n", pf.token());
              return static_cast<int>(1000 - pf.token());
            }
          }),

      tf::make_data_pipe<int, std::string>(
          tf::PipeType::SERIAL,
          [](int& input) {
            printf("222 second pipe returns a strong of %d\n", input + 100);
            return std::to_string(input + 100);
          }),

      tf::make_data_pipe<std::string, void>(
          tf::PipeType::SERIAL, [](std::string& input) {
            printf("*** third pipe receives the input string %s\n",
                   input.c_str());
          }));

  // build the pipeline graph using composition
  taskflow.composed_of(pl).name("pipeline");

  // dump the pipeline graph structure (with composition)
  taskflow.dump(std::cout);

  // run the pipeline
  executor.run(taskflow).wait();

  return 0;
}