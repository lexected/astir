#pragma once

struct MachineDefinition;
struct MachineStatement;
struct Field;
class IActing {
public:
	virtual void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) { };

protected:
	IActing() = default;
};