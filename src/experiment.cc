#include <IOHprofiler_experimenter.h>
#include <random>
#include <set>
#include "hybridalgorithm.h"
#include "differentialevolution.h"
#include "hybridsuite.h"
HybridAlgorithm* ha;
ParticleSwarm* pso;
DifferentialEvolution* de;

void algorithm
(std::shared_ptr<IOHprofiler_problem<double>> problem,
 std::shared_ptr<IOHprofiler_csv_logger> logger) {
    int const D = problem->IOHprofiler_get_number_of_variables(); 
    pso->run(problem, logger, D*10000, D*5, std::map<int,double>()); 
}


void _run_experiment(bool log) {
    pso = new ParticleSwarm(DECR_INERTIA_WEIGHT, GBEST, ASYNCHRONOUS);

    if (log)
	pso->enableLogging();

    std::string configName = "./configuration.ini";
    IOHprofiler_experimenter<double> experimenter(configName,algorithm); 
    experimenter._set_independent_runs(5);
    experimenter._run();
    delete pso;
}

int main(int argc, char** argv){
    bool log = false;
    _run_experiment(log);	
}
