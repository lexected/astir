#pragma once

class Machine;
class MachineComponent;
struct Field;
class IActing {
public:
	virtual void checkAndTypeformActionUsage(const Machine& machine, const MachineComponent* context) { };

protected:
	IActing() = default;
};