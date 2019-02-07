//
// Created by simengangstad on 11.10.18.
//

#ifndef FLUID_FSM_IDLE_STATE_H
#define FLUID_FSM_IDLE_STATE_H

#include "../core/state.h"
#include "../mavros/mavros_state.h"
#include "state_defines.h"

#include <ros/ros.h>

// TODO: Use pixhawk idle state

namespace fluid {

    /** \class IdleState
     *  \brief Represents the state where the drone is on ground, armed and spinning its rotors
     */
    class IdleState: public MavrosState {
    public:

        /**
         * Initializes the idle state.
         */
        explicit IdleState() : MavrosState(fluid::state_identifiers::IDLE) {}

        /**
         * Overridden function. @see State::hasFinishedExecution
         */
        bool hasFinishedExecution() override;

        /**
         * Overridden function. @see State::tick
         */
        void tick() override;
    };
}

#endif //FLUID_FSM_IDLE_STATE_H
