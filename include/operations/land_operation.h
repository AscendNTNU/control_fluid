//
// Created by simengangstad on 08.11.18.
//

#ifndef FLUID_FSM_LAND_OPERATION_H
#define FLUID_FSM_LAND_OPERATION_H

#include "../core/operation/operation.h"
#include "../core/state.h"
#include "../core/transition.h"
#include "operation_defines.h"
#include <mavros_msgs/PositionTarget.h>
#include "../states/state_defines.h"

namespace fluid {

    /**
     * \class LandOperation
     * \brief Encapsulates the operation of landing at the current position.
     */
    class LandOperation: public Operation {

    public:

        LandOperation(mavros_msgs::PositionTarget position_target, unsigned int refresh_rate) :
        Operation(fluid::operation_identifiers::LAND,
                  fluid::state_identifiers::LAND,
                  fluid::state_identifiers::IDLE,
                  position_target,
                  refresh_rate) {}

        /**
         * Method overriden from superclass.
         */
        bool validateOperationFromCurrentState(std::shared_ptr<fluid::State> current_state_p);
    };
}

#endif //FLUID_FSM_LAND_OPERATION_H
