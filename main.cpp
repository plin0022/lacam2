#include <argparse/argparse.hpp>
#include <lacam2.hpp>

int main(int argc, char* argv[])
{
  // arguments parser
  argparse::ArgumentParser program("lacam2", "0.1.0");
  program.add_argument("-m", "--map").help("map file").required();
  program.add_argument("-i", "--scen")
      .help("scenario file")
      .default_value(std::string(""));
  program.add_argument("-N", "--num").help("number of agents").required();
  program.add_argument("-s", "--seed")
      .help("seed")
      .default_value(std::string("0"));
  program.add_argument("-v", "--verbose")
      .help("verbose")
      .default_value(std::string("0"));
  program.add_argument("-t", "--time_limit_sec")
      .help("time limit sec")
      .default_value(std::string("3"));
  program.add_argument("-o", "--output")
      .help("output file")
      .default_value(std::string("./build/result.txt"));
  program.add_argument("-l", "--log_short")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-O", "--objective")
      .help("0: none, 1: makespan, 2: sum_of_loss")
      .default_value(std::string("0"))
      .action([](const std::string& value) {
        static const std::vector<std::string> C = {"0", "1", "2"};
        if (std::find(C.begin(), C.end(), value) != C.end()) return value;
        return std::string("0");
      });
  program.add_argument("-r", "--restart_rate")
      .help("restart rate")
      .default_value(std::string("0.001"));

  try {
    program.parse_known_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  // setup instance
  const auto verbose = std::stoi(program.get<std::string>("verbose"));
  const auto time_limit_sec =
      std::stoi(program.get<std::string>("time_limit_sec"));
  //  const auto scen_name = program.get<std::string>("scen");
  const auto seed = std::stoi(program.get<std::string>("seed"));
  auto MT = std::mt19937(seed);
  //  const auto map_name = program.get<std::string>("map");
  const auto output_name = program.get<std::string>("output");
  const auto log_short = program.get<bool>("log_short");
  //  const auto N = std::stoi(program.get<std::string>("num"));
  //  const auto ins = scen_name.size() > 0 ? Instance(scen_name, map_name, N)
  //                                        : Instance(map_name, &MT, N);
  const auto objective =
      static_cast<Objective>(std::stoi(program.get<std::string>("objective")));
  const auto restart_rate = std::stof(program.get<std::string>("restart_rate"));
  //  if (!ins.is_valid(1)) return 1;




  // set up parameters for generating charts

  // scenarios
  std::string scen_name;
//  std::string base_path = "assets/scen-random_random-32-32-20/random-32-32-20-random-";
//  std::string base_path = "assets/scen-random_warehouse-10-20-10-2-1/warehouse-10-20-10-2-1-random-";
//  std::string base_path = "assets/scen-random_random-64-64-20/random-64-64-20-random-";
//  std::string base_path = "assets/scen-random_maze-128-128-1/maze-128-128-1-random-";
//  std::string base_path = "assets/scen-random_den312d/den312d-random-";
  std::string base_path = "assets/scen-random_empty-48-48/empty-48-48-random-";


  // map
//  std::string map_name = "assets/random-32-32-20.map";
//  std::string map_name = "assets/warehouse-10-20-10-2-1.map";
//  std::string map_name = "assets/random-64-64-20.map";
//  std::string map_name = "assets/maze-128-128-1.map";
//  std::string map_name = "assets/den312d.map";
  std::string map_name = "assets/empty-48-48.map";




  using VarType = std::variant<int, float>;
  std::vector<std::vector<VarType>> obj_table;


  // objectives
  volatile float temp_soc = 0;
  volatile float temp_soc_lb = 0;
  volatile float temp_makespan = 0;
  volatile float temp_makespan_lb = 0;
  volatile float temp_time = 0;


  volatile float sum_soc_over_lb = 0;
  volatile float sum_makespan_over_lb = 0;
  volatile float sum_time = 0;
  volatile float sum_success = 0;


  int max_num_agents = 1000;
  int max_scen_num = 25;

  for (int num_of_agents = 10; num_of_agents <= max_num_agents; num_of_agents = num_of_agents + 10)
  {
    // initialize parameters
    sum_soc_over_lb = 0;
    sum_makespan_over_lb = 0;
    sum_time = 0;
    sum_success = 0;


    for (int scen_num = 1; scen_num <= max_scen_num; ++scen_num)
    {
      scen_name = base_path + std::to_string(scen_num) + ".scen";

      auto ins = scen_name.size() > 0
                     ? Instance(scen_name, map_name, num_of_agents)
                     : Instance(map_name, &MT, num_of_agents);
      if (!ins.is_valid(1)) return 1;

      // solve
      auto additional_info = std::string("");
      auto deadline = Deadline(time_limit_sec * 1000);
      auto solution = solve(ins, additional_info, verbose - 1, &deadline,
                            &MT, objective, restart_rate);
      auto comp_time_ms = deadline.elapsed_ms();

      // failure
      if (solution.empty())
      {
        info(1, verbose, "failed to solve");
      }

//      // check feasibility
//      if (!is_feasible_solution(ins, solution, verbose)) {
//        info(0, verbose, "invalid solution");
//      }

      // post processing
      print_stats(verbose, ins, solution, comp_time_ms);


      auto dist_table = DistTable(ins);
      temp_soc = (float)get_sum_of_costs(solution);
      temp_soc_lb = (float)get_sum_of_costs_lower_bound(ins, dist_table);
      temp_makespan = (float)get_makespan(solution);
      temp_makespan_lb = (float)get_makespan_lower_bound(ins, dist_table);
      temp_time = comp_time_ms;

      // record successful data
      if (!solution.empty() && temp_soc != 0)
      {
        sum_success = sum_success + 1;

        sum_soc_over_lb = sum_soc_over_lb + temp_soc / temp_soc_lb;
        sum_makespan_over_lb = sum_makespan_over_lb + temp_makespan / temp_makespan_lb;
        sum_time = sum_time + temp_time;
      }
    }

    std::vector<VarType> temp_list;
    temp_list.push_back(num_of_agents);
    temp_list.push_back(sum_success / max_scen_num);
    temp_list.push_back(sum_time / sum_success);
    temp_list.push_back(sum_soc_over_lb / sum_success);
    temp_list.push_back(sum_makespan_over_lb / sum_success);

    obj_table.push_back(temp_list);
  }



  // print out the test results

  // Create an output file stream for the CSV file
  std::ofstream outfile("output.csv");

  // Check if the file is opened successfully
  if (!outfile.is_open()) {
    std::cerr << "Failed to open the file." << std::endl;
    return 1;
  }

  // Set precision for floating point numbers
  outfile << std::fixed << std::setprecision(5);

  // Iterate over the rows of the table
  for (const auto& row : obj_table) {
    // Iterate over the elements in each row
    for (size_t i = 0; i < row.size(); ++i) {
      // Use std::visit to handle the different types in VarType
      std::visit([&outfile](auto&& arg) {
        outfile << arg;
      }, row[i]);
      // Add a comma if it's not the last element in the row
      if (i < row.size() - 1) {
        outfile << ",";
      }
    }
    // Print a new line after each row
    outfile << std::endl;
  }

  // Close the file
  outfile.close();



  //  make_log(ins, solution, output_name, comp_time_ms, map_name, seed,
  //           additional_info, log_short);
  return 0;
}
