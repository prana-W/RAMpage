#pragma once

#include "../pubsub/PubSubManager.h"
#include "CommandRegistry.h"

void registerPubSubCommands(CommandRegistry& reg, PubSubManager& pubSub);
