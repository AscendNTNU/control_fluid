//
// Created by simengangstad on 25.10.18.
//

#ifndef FLUID_FSM_MOVE_OPERATION_H
#define FLUID_FSM_MOVE_OPERATION_H

#include "../core/operation/operation.h"
#include "../core/state.h"
#include "../core/transition.h"
#include "operation_defines.h"
#include "../states/state_defines.h"
#include <mavros_msgs/PositionTarget.h>

namespace fluid {

    /**
     * \class MoveOperation
     * \brief Encapsulates the operation of moving from a to b.
     */
    class MoveOperation: public Operation {

    public:

        MoveOperation(mavros_msgs::PositionTarget position_target, unsigned int refresh_rate) :
        Operation(fluid::operation_identifiers::MOVE,
                  fluid::state_identifiers::MOVE,
                  fluid::state_identifiers::HOLD,
                  position_target,
                  refresh_rate) {}

        /**
         * Method overriden from superclass.
         */
        bool validateOperationFromCurrentState(std::shared_ptr<fluid::State> current_state_p);
    };
}

#endif //FLUID_FSM_MOVE_OPERATION_H
