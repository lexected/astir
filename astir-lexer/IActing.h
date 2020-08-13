#pragma once

class MachineComponent;
class IActing {
public:
	virtual void checkActionUsage(const MachineComponent* context) const { };

protected:
	IActing() = default;
};