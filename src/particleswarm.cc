#include <IOHprofiler_problem.h>
#include <IOHprofiler_csv_logger.h>
#include "particleswarm.h"
#include "particle.h"
#include "topologymanager.h"
#include "particleupdatesettings.h"
#include "instancenamer.h"
#include "repairhandler.h"
#include <limits>
#include <iostream>
#include <fstream>

ParticleSwarm::ParticleSwarm(UpdateManagerType const updateManagerType,
	Topology topologyManagerType, Synchronicity const synchronicity):

	updateManagerType(updateManagerType),
	topologyManagerType(topologyManagerType), topologyManager(NULL), synchronicity(synchronicity){

	logging = false;
}

void ParticleSwarm::reset(){
	delete topologyManager;
	topologyManager = NULL;
	
	for (Particle* const particle : particles)
		delete particle;

	particles.clear();

	delete psoCH;
}

ParticleSwarm::~ParticleSwarm(){
	if (topologyManager != NULL)
		delete topologyManager;

	if (!particles.empty())
		for (Particle* const particle : particles)
			delete particle;

}

void ParticleSwarm::run(std::shared_ptr<IOHprofiler_problem<double> > problem, 
    		std::shared_ptr<IOHprofiler_csv_logger> logger,
    		int const evalBudget, int popSize, std::map<int,double> particleUpdateParams){

	this->problem=problem;
	this->logger=logger;

	if (synchronicity == SYNCHRONOUS)
		runSynchronous(evalBudget, popSize, particleUpdateParams);
	else if (synchronicity == ASYNCHRONOUS)
		runAsynchronous(evalBudget, popSize, particleUpdateParams);
}

void ParticleSwarm::runAsynchronous(int const evalBudget, 
			int popSize, std::map<int,double> particleUpdateParams){
	this->topologyManager = TopologyManager::createTopologyManager(topologyManagerType, particles);
	popSize = this->topologyManager->getClosestValidPopulationSize(popSize);

	int const D = problem->IOHprofiler_get_number_of_variables(); /// dimension
	std::vector<double> smallest = problem->IOHprofiler_get_lowerbound(); //??
	std::vector<double> largest = problem->IOHprofiler_get_upperbound(); //??

	psoCH = new HyperbolicRepair(smallest, largest); 
	ParticleUpdateSettings settings(updateManagerType, particleUpdateParams, psoCH);

	for (int i = 0; i < popSize; i++){
		Particle* p = new Particle(D, settings);
		p->randomize(smallest, largest);
		particles.push_back(p);
	}

	logStart();
	logPositions();

	topologyManager->initialize();
	
	int iterations = 0;
	double bestFitness = std::numeric_limits<double>::max();
	int notImproved = 0;
	bool improved;

	while (	
			//notImproved < 100 && 
			problem->IOHprofiler_get_evaluations() < evalBudget &&
			!problem->IOHprofiler_hit_optimal()){
		
		improved = false;

		for (int i = 0; i < popSize; i++){
			std::vector<double> position = particles[i]->getX();			
			double y = particles[i]->evaluate(problem,logger);

			if (y < bestFitness){
				improved = true;
				bestFitness = y;
			}

			particles[i]->updatePbest();
			particles[i]->updateGbest();
			particles[i]->updateVelocityAndPosition(double(problem->IOHprofiler_get_evaluations())/double(evalBudget));			
		}
		logPositions();

		improved ? notImproved=0 : notImproved++;

		iterations++;	
		topologyManager->update(double(problem->IOHprofiler_get_evaluations())/double(evalBudget));	
	}

	reset();
	logEnd();
}

void ParticleSwarm::runSynchronous(int const evalBudget, int popSize, 
			std::map<int,double> particleUpdateParams){

	this->topologyManager = TopologyManager::createTopologyManager(topologyManagerType, particles);
	popSize = this->topologyManager->getClosestValidPopulationSize(popSize);

	int const D = problem->IOHprofiler_get_number_of_variables(); /// dimension
	std::vector<double> smallest = problem->IOHprofiler_get_lowerbound(); //??
	std::vector<double> largest = problem->IOHprofiler_get_upperbound(); //??

	ConstraintHandler* psoCH = new HyperbolicRepair(smallest, largest); 
	ParticleUpdateSettings settings(updateManagerType, particleUpdateParams, psoCH);

	for (int i = 0; i < popSize; i++){
		Particle* p = new Particle(D, settings);
		p->randomize(smallest, largest);
		particles.push_back(p);
	}

	logStart();
	logPositions();

	topologyManager->initialize();
	
	int iterations = 0;
	
	double bestFitness = std::numeric_limits<double>::max();
	int notImproved = 0;
	bool improved;

	while (	//notImproved < 100 && 
			problem->IOHprofiler_get_evaluations() < evalBudget &&
			!problem->IOHprofiler_hit_optimal()){
		
		improved = false;

		for (int i = 0; i < popSize; i++){		
			std::vector<double> position = particles[i]->getX();			
			double y = particles[i]->evaluate(problem,logger);

			if (y < bestFitness){
				improved = true;
				bestFitness = y;
			}	
		}		

		improved ? notImproved=0 : notImproved++;
		
		for (int i = 0; i < popSize; i++)
			particles[i]->updatePbest();

		for (int i = 0; i < popSize; i++)
			particles[i]->updateGbest();

		for (int i = 0; i < popSize; i++)
			particles[i]->updateVelocityAndPosition(double(problem->IOHprofiler_get_evaluations())/double(evalBudget));
	
		logPositions();
	
		iterations++;	
		topologyManager->update(double(problem->IOHprofiler_get_evaluations())/double(evalBudget));	
	}

	reset();
	logEnd();
}

std::string ParticleSwarm::getIdString() const {
	return InstanceNamer::getName(updateManagerType, topologyManagerType, synchronicity);
}

void ParticleSwarm::enableLogging(){
	logging=true;
}

void ParticleSwarm::logStart(){
	if (logging){
		std::cout << "START" << std::endl;
		// TODO: information about run
	}
}

void ParticleSwarm::logEnd(){
	if (logging){
		std::cout << "END" << std::endl;
	}
}

void ParticleSwarm::logPositions(){
	if (logging){
		std::cout << std::endl;
		for (unsigned int i = 0; i < particles.size(); i++){
			std::vector<double> position = particles[i]->getX();
			for (unsigned int j = 0; j < position.size()-1 ; j++){
				std::cout << position[j] << " ";
			}
			std::cout << position[position.size()-1] << std::endl;
		}
	}
}
