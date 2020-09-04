#pragma once
#include <vector>

class ConstraintHandler {
	public:
		virtual ~ConstraintHandler();
		// Generic constraint handler
		virtual void repair(std::vector<double>& x)=0;
};

class DEConstraintHandler : public ConstraintHandler{
	public:
		virtual ~DEConstraintHandler();
		// Generic constraint handler
		virtual void repair(std::vector<double>& x)=0;
};

class PSOConstraintHandler : public ConstraintHandler {
	public:
		virtual ~PSOConstraintHandler();
		// Generic constraint handler
		virtual void repair(std::vector<double>& x)=0;
};