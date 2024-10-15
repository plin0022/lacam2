#include "../include/dist_table.hpp"

DistTable::DistTable(const Instance& ins)
    : V_size(ins.G.V.size()), table(ins.N, std::vector<uint>(V_size, V_size))
{
  setup(&ins);
}

DistTable::DistTable(const Instance* ins)
    : V_size(ins->G.V.size()), table(ins->N, std::vector<uint>(V_size, V_size))
{
  setup(ins);
}

FlexTable::FlexTable(const Instance* ins)
    : V_size(ins->G.V.size()),
      table(ins->N,
            std::vector<uint>(V_size,
                              std::numeric_limits<unsigned int>::max()))
{
  setup(ins);
}

void DistTable::setup(const Instance* ins)
{
  for (size_t i = 0; i < ins->N; ++i) {
    OPEN.push_back(std::queue<Vertex*>());
    auto n = ins->goals[i];
    OPEN[i].push(n);
    table[i][n->id] = 0;
  }
}

void FlexTable::setup(const Instance* ins)
{
  for (size_t i = 0; i < ins->N; ++i) {
    auto n = ins->goals[i];
    table[i][n->id] = 0;
  }
}

uint DistTable::get(uint i, uint v_id)
{
  if (table[i][v_id] < V_size) return table[i][v_id];

  /*
   * BFS with lazy evaluation
   * c.f., Reverse Resumable A*
   * https://www.aaai.org/Papers/AIIDE/2005/AIIDE05-020.pdf
   *
   * sidenote:
   * tested RRA* but lazy BFS was much better in performance
   */

  while (!OPEN[i].empty()) {
    auto&& n = OPEN[i].front();
    OPEN[i].pop();
    const int d_n = table[i][n->id];
    for (auto&& m : n->neighbor) {
      const int d_m = table[i][m->id];
      if (d_n + 1 >= d_m) continue;
      table[i][m->id] = d_n + 1;
      OPEN[i].push(m);
    }
    if (n->id == v_id) return d_n;
  }
  return V_size;
}

uint FlexTable::get(uint i, Vertex* v, DistTable& distTable, int step)
{
  if (table[i][v->id] < std::numeric_limits<unsigned int>::max()) return table[i][v->id];

//  if (step >= 1) return 0;
//  if (step >= 5) return 0;
//  if (step >= 10) return 0;

  const uint curr_dis = distTable.get(i, v);
  int award_points = 2;
  uint final_points = 0;

  for (auto a_node : v->neighbor)
  {
    if (distTable.get(i, a_node) < curr_dis)
      final_points = final_points + award_points +
                     get(i, a_node, distTable, step + 1);
  }

  table[i][v->id] = final_points;

  return final_points;
}

uint DistTable::get(uint i, Vertex* v) { return get(i, v->id); }



