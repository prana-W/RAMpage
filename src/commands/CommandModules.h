#pragma once

#include "CommandRegistry.h"

// This file contains the declarations for all our command modules.
void registerStringCommands(CommandRegistry& reg);
void registerListCommands(CommandRegistry& reg);
void registerPubSubCommands(CommandRegistry& reg, class PubSubManager& pubSub);