#pragma once

class Machine;
class MachineComponent;
struct Field;
class IActing {
public:
	virtual void checkActionUsage(const Machine& machine, const MachineComponent* context) const { };

protected:
	IActing() = default;
};