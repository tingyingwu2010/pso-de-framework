#include "iohsrc/Template/Experiments/IOHprofiler_experimenter.hpp"
#include <random>
#include <set>
#include "hybridalgorithm.h"
#include "differentialevolution.h"

HybridAlgorithm* ha;
ParticleSwarm* pso;
DifferentialEvolution* de;

void algorithm
	(std::shared_ptr<IOHprofiler_problem<double>> problem,
		std::shared_ptr<IOHprofiler_csv_logger> logger) {
	int const D = problem->IOHprofiler_get_number_of_variables();

	pso->run(problem, logger, D*10000, D*5, std::map<int,double>()); 
}


void _run_experiment() {
       pso = new ParticleSwarm(INERTIA_WEIGHT, LBEST, ASYNCHRONOUS);

      std::string configName = "./configuration.ini";
      IOHprofiler_experimenter<double> experimenter(configName,"PSO",algorithm); 
      experimenter._set_independent_runs(5);
      experimenter._run();
      delete pso;
}

int main(){
  _run_experiment();
  return 0; 
}