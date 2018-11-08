//
// Created by simengangstad on 08.11.18.
//

#ifndef FLUID_FSM_LAND_OPERATION_H
#define FLUID_FSM_LAND_OPERATION_H

#include "../core/operation/operation.h"
#include "../core/state.h"
#include "../core/transition.h"
#include <mavros_msgs/PositionTarget.h>

namespace fluid {

    /**
     * \class LandOperation
     * \brief Encapsulates the operation of landing at the current position.
     */
    class LandOperation: public Operation {

    public:

        LandOperation(mavros_msgs::PositionTarget position_target) :
        Operation("land_operation", "land", "idle", position_target) {}

        /**
         * Method overriden from superclass.
         */
        bool validateOperationFromCurrentState(std::shared_ptr<fluid::State> current_state_p);
    };
}

#endif //FLUID_FSM_LAND_OPERATION_H
