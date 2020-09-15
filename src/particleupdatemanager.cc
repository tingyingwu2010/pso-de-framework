#include <vector>
#include <random>
#include <algorithm>
#include <functional>
#include <iostream>
#include <math.h>
#include "particle.h"
#include "particleupdatemanager.h"
#include "particleupdatesettings.h"
#include "util.h"
#include "rng.h"

/*		Base 		*/
ParticleUpdateManager::ParticleUpdateManager(std::vector<double>& x, std::vector<double>& v,
	std::vector<double> & p, std::vector<double>& g)
	:x(x), v(v), p(p), g(g), D(x.size()){
}

ParticleUpdateManager::~ParticleUpdateManager(){}

void ParticleUpdateManager::updatePosition(){
		std::transform (x.begin(), x.end(),
				v.begin(), x.begin(),
		std::plus<double>());
}

ParticleUpdateManager* ParticleUpdateManager::createParticleUpdateManager
	(std::vector<double>& x, std::vector<double>& v,std::vector<double> & p, 
	std::vector<double>& g,  ParticleUpdateSettings const settings, std::vector<Particle*>& neighborhood){

	switch(settings.managerType){
		case UpdateManagerType::INERTIA_WEIGHT:
			return new InertiaWeightManager(x,v,p,g,settings.parameters);
		case UpdateManagerType::DECR_INERTIA_WEIGHT:
			return new DecrInertiaWeightManager(x,v,p,g,settings.parameters);
		case UpdateManagerType::CONSTRICTION_COEFFICIENT:
			return new ConstrictionCoefficientManager(x,v,p,g,settings.parameters);
		case UpdateManagerType::FIPS:
			return new FIPSManager(x,v,p,g,settings.parameters, neighborhood);
		case UpdateManagerType::BARE_BONES:
			return new BareBonesManager(x,v,p,g,settings.parameters);
		default: 
			throw std::invalid_argument("Error: Invalid particle update manager type");
	}
}

/*		Inertia weight 		*/
InertiaWeightManager::InertiaWeightManager (std::vector<double>& x, std::vector<double>& v,
	std::vector<double> & p, std::vector<double>& g,  std::map<int, double> parameters)
	: ParticleUpdateManager(x,v,p,g),
	phi1 (parameters.find(Setting::S_INER_PHI1) != parameters.end() ? parameters[Setting::S_INER_PHI1] : INER_PHI1_DEFAULT),
	phi2 (parameters.find(Setting::S_INER_PHI2) != parameters.end() ? parameters[Setting::S_INER_PHI2] : INER_PHI2_DEFAULT),	
	w (parameters.find(Setting::S_INER_W) != parameters.end() ? parameters[Setting::S_INER_W] : INER_W_DEFAULT){
}

void InertiaWeightManager::updateVelocity(double progress) {
	std::vector<double> pMinx(D);
	subtract(p,x,pMinx);
	std::vector<double> gMinx(D);	
	subtract(g,x,gMinx);

	randomMult(pMinx, 0, phi1);
	randomMult(gMinx, 0, phi2);	
	scale(v, w);
	add(v,pMinx, v);
	add(v,gMinx, v);
}

/*	Decreasing inertia weight manager */
DecrInertiaWeightManager::DecrInertiaWeightManager (std::vector<double>& x, std::vector<double>& v,
	std::vector<double> & p, std::vector<double>& g,  std::map<int, double> parameters)
	: ParticleUpdateManager(x,v,p,g),
	phi1 (parameters.find(Setting::S_DINER_PHI1) != parameters.end() ? parameters[Setting::S_DINER_PHI1] : DINER_PHI2_DEFAULT),
	phi2 (parameters.find(Setting::S_DINER_PHI2) != parameters.end() ? parameters[Setting::S_DINER_PHI2] : DINER_PHI2_DEFAULT),	
	w (parameters.find(Setting::S_DINER_W_START) != parameters.end() ? parameters[Setting::S_DINER_W_START] : DINER_W_START_DEFAULT),
	wMin (parameters.find(Setting::S_DINER_W_END) != parameters.end() ? parameters[Setting::S_DINER_W_END] : DINER_W_END_DEFAULT),
	wMax(w){

}

void DecrInertiaWeightManager::updateVelocity(double progress) {
	std::vector<double> pMinx(D);
	subtract(p,x,pMinx);
	std::vector<double> gMinx(D);
	subtract(g,x,gMinx);
	randomMult(pMinx, 0, phi1);
	randomMult(gMinx, 0, phi2);
	scale(v,w);
	add(v,pMinx, v);
	add(v,gMinx,v);

	w = wMax - progress * (wMax - wMin);
}

/*		Constriction Coefficient 		*/
ConstrictionCoefficientManager::ConstrictionCoefficientManager(std::vector<double> & x, std::vector<double> & v,
	std::vector<double> & p, std::vector<double> & g,  std::map<int, double> parameters)
	: ParticleUpdateManager(x,v,p,g),
	phi1 (parameters.find(Setting::S_CC_PHI1) != parameters.end() ? parameters[Setting::S_CC_PHI1] : CC_PHI1_DEFAULT),
	phi2 (parameters.find(Setting::S_CC_PHI2) != parameters.end() ? parameters[Setting::S_CC_PHI2] : CC_PHI2_DEFAULT),
	chi (2 / ((phi1+phi2) -2 + sqrt( pow(phi1+phi2, 2) - 4 * (phi1+phi2)))){
}

void ConstrictionCoefficientManager::updateVelocity(double progress){
	std::vector<double> pMinx(D);
	pMinx.resize(D);
	subtract(p,x,pMinx);
	std::vector<double> gMinx(D);
	subtract(g,x,gMinx);
	randomMult(pMinx, 0, phi1);
	randomMult(gMinx, 0, phi2);
	add(v,pMinx,v);
	add(v,gMinx,v);
	scale(v, chi);
}

/*		Fully Informed 		*/
FIPSManager::FIPSManager(std::vector<double> & x, std::vector<double> & v,
	std::vector<double> & p, std::vector<double> & g,  std::map<int, double> parameters
	, std::vector<Particle*>& neighborhood)
	: ParticleUpdateManager(x,v,p,g),
	phi (parameters.find(Setting::S_FIPS_PHI) != parameters.end() ? parameters[Setting::S_FIPS_PHI] : FIPS_PHI_DEFAULT),
	chi (2 / ((phi) -2 + sqrt( pow(phi, 2) - 4 * (phi)))),
	neighborhood(neighborhood){
}


void FIPSManager::updateVelocity(double progress){
	double const oneOverK = 1.0/neighborhood.size();

	std::vector<double> sum(D,0.0);
	std::vector<double> pMinx(D);

	std::vector< std::vector<double> > p_n;
	p_n.reserve(neighborhood.size());
	for (Particle* n : neighborhood)
		p_n.push_back(n->getP());

	for (int i = 0; i < (int) neighborhood.size(); i++){
		subtract(p_n[i], x, pMinx);
		scale(pMinx, rng.randDouble(0,phi));
		add(sum, pMinx, sum);		
	}

	scale(sum, oneOverK);
	add(v,sum,v);
	scale(v, chi);
}

/* 		Bare Bones 		*/
BareBonesManager::BareBonesManager(std::vector<double> & x, std::vector<double> & v,
	std::vector<double> & p, std::vector<double> & g,  std::map<int, double> parameters) :
	ParticleUpdateManager(x,v,p,g) {

}

void BareBonesManager::updateVelocity(double progress){
	for (int i = 0; i < D; i++){
		x[i] = rng.normalDistribution((g[i] + p[i]) / 2.0, fabs(g[i] - p[i]));
	}
}
