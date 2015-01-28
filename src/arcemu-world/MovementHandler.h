/*
 * ArcEmu MMORPG Server
 * Copyright (C) 2012 <http://www.ArcEmu.org/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

enum MovementStatusElements
{
	MSEHasGuidByte0,
	MSEHasGuidByte1,
	MSEHasGuidByte2,
	MSEHasGuidByte3,
	MSEHasGuidByte4,
	MSEHasGuidByte5,
	MSEHasGuidByte6,
	MSEHasGuidByte7,
	MSEHasMovementFlags,
	MSEHasMovementFlags2,
	MSEHasTimestamp,
	MSEHasOrientation,
	MSEHasTransportData,
	MSEHasTransportGuidByte0,
	MSEHasTransportGuidByte1,
	MSEHasTransportGuidByte2,
	MSEHasTransportGuidByte3,
	MSEHasTransportGuidByte4,
	MSEHasTransportGuidByte5,
	MSEHasTransportGuidByte6,
	MSEHasTransportGuidByte7,
	MSEHasTransportTime2,
	MSEHasTransportTime3,
	MSEHasPitch,
	MSEHasFallData,
	MSEHasFallDirection,
	MSEHasSplineElevation,
	MSEHasSpline,

	MSEGuidByte0,
	MSEGuidByte1,
	MSEGuidByte2,
	MSEGuidByte3,
	MSEGuidByte4,
	MSEGuidByte5,
	MSEGuidByte6,
	MSEGuidByte7,
	MSEMovementFlags,
	MSEMovementFlags2,
	MSETimestamp,
	MSEPositionX,
	MSEPositionY,
	MSEPositionZ,
	MSEOrientation,
	MSETransportGuidByte0,
	MSETransportGuidByte1,
	MSETransportGuidByte2,
	MSETransportGuidByte3,
	MSETransportGuidByte4,
	MSETransportGuidByte5,
	MSETransportGuidByte6,
	MSETransportGuidByte7,
	MSETransportPositionX,
	MSETransportPositionY,
	MSETransportPositionZ,
	MSETransportOrientation,
	MSETransportSeat,
	MSETransportTime,
	MSETransportTime2,
	MSETransportTime3,
	MSEPitch,
	MSEFallTime,
	MSEFallVerticalSpeed,
	MSEFallCosAngle,
	MSEFallSinAngle,
	MSEFallHorizontalSpeed,
	MSESplineElevation,

	MSECounter,
	MSECounterCount,
	MSEUintCount,
	MSEHasUnkTime,
	MSEUnkTime,

	// Special
	MSEZeroBit,         // writes bit value 1 or skips read bit
	MSEOneBit,          // writes bit value 0 or skips read bit
	MSEEnd,             // marks end of parsing
	MSEExtraElement,    // Used to signalize reading into ExtraMovementStatusElement, element sequence inside it is declared as separate array
	// Allowed internal elements are: GUID markers (not transport), MSEExtraFloat, MSEExtraInt8
	MSEExtraFloat,
	MSEExtraInt8,
    MSE_COUNT
};

MovementStatusElements PlayerMoveSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementFallLandSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementHeartBeatSequence[] =
{
	MSEPositionZ,              // 44
	MSEPositionX,              // 36
	MSEPositionY,              // 40
	MSECounterCount,            // 152
	MSEHasMovementFlags,       // 24
	MSEZeroBit,                // 148
	MSEHasUnkTime,             // 168
	MSEHasGuidByte3,           // 19
	MSEHasGuidByte6,           // 22
	MSEHasPitch,               // 112
	MSEZeroBit,                // 149
	MSEZeroBit,                // 172
	MSEHasGuidByte7,           // 23
	MSEHasGuidByte2,           // 18
	MSEHasGuidByte4,           // 20
	MSEHasMovementFlags2,      // 28
	MSEHasOrientation,         // 48
	MSEHasTimestamp,           // 32
	MSEHasTransportData,       // 104
	MSEHasFallData,            // 140
	MSEHasGuidByte5,           // 21
	MSEHasSplineElevation,     // 144
	MSEHasGuidByte1,           // 17
	MSEHasGuidByte0,           // 16
	MSEHasTransportGuidByte5,  // 61
	MSEHasTransportGuidByte3,  // 59
	MSEHasTransportGuidByte6,  // 62
	MSEHasTransportGuidByte0,  // 56
	MSEHasTransportGuidByte7,  // 63
	MSEHasTransportTime3,      // 100
	MSEHasTransportGuidByte1,  // 57
	MSEHasTransportGuidByte2,  // 58
	MSEHasTransportGuidByte4,  // 60
	MSEHasTransportTime2,      // 92
	MSEMovementFlags,          // 24
	MSEHasFallDirection,       // 136
	MSEMovementFlags2,         // 28
	MSEGuidByte2,              // 18
	MSEGuidByte3,              // 19
	MSEGuidByte6,              // 22
	MSEGuidByte1,              // 17
	MSEGuidByte4,              // 20
	MSEGuidByte7,              // 23
	MSECounter,                 // 156
	MSEGuidByte5,              // 21
	MSEGuidByte0,              // 16
	MSEFallSinAngle,           // 128
	MSEFallCosAngle,           // 124
	MSEFallHorizontalSpeed,    // 132
	MSEFallVerticalSpeed,      // 120
	MSEFallTime,               // 116
	MSETransportGuidByte1,     // 57
	MSETransportGuidByte3,     // 59
	MSETransportGuidByte2,     // 58
	MSETransportGuidByte0,     // 56
	MSETransportTime3,         // 96
	MSETransportSeat,          // 80
	MSETransportGuidByte7,     // 63
	MSETransportPositionX,     // 64
	MSETransportGuidByte4,     // 60
	MSETransportTime2,         // 88
	MSETransportPositionY,     // 68
	MSETransportGuidByte6,     // 62
	MSETransportGuidByte5,     // 61
	MSETransportPositionZ,     // 72
	MSETransportTime,          // 84
	MSETransportOrientation,   // 76
	MSECounter,                // 168
	MSEOrientation,            // 48
	MSEPitch,                  // 112
	MSETimestamp,              // 32
	MSESplineElevation,        // 144
	MSEEnd
};

MovementStatusElements MovementJumpSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementSetFacingSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementSetPitchSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementStartBackwardSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartForwardSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartStrafeLeftSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartStrafeRightSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartTurnLeftSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartTurnRightSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStopSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStopStrafeSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStopTurnSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementStartAscendSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementStartDescendSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartSwimSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementStopSwimSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStopAscendSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementStopPitchSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartPitchDownSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementStartPitchUpSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementChngTransportSequence[]=
{
    
    MSEEnd,
};

MovementStatusElements MovementSetRunModeSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementSetWalkModeSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementFallResetSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementSetCanFlyAckSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MovementCastSpellSequence[] =
{
   
    MSEEnd,
};

MovementStatusElements MovementSplineDoneSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MoveKnockbackAckSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MoveUpdateKnockBackSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements MoveNotActiveMoverSequence[] =
{
    
    MSEEnd,
};

MovementStatusElements* GetMovementStatusElementsSequence(uint16 opcode)
{
    switch(opcode)
    {
		//!!! UNCOMMENT THIS ONCE I HAVE IMPLEMENTED MOVEMENT OPCODES

       /* case CMSG_CAST_SPELL:
        case CMSG_PET_CAST_SPELL:
        case CMSG_USE_ITEM:
            return MovementCastSpellSequence;
        case CMSG_MOVE_CHNG_TRANSPORT:
            return MovementChngTransportSequence;
        case CMSG_MOVE_FALL_LAND:
            return MovementFallLandSequence;
        case CMSG_MOVE_FALL_RESET:
            return MovementFallResetSequence;
        case CMSG_MOVE_JUMP:
            return MovementJumpSequence;
        case CMSG_MOVE_SET_CAN_FLY_ACK:
            return MovementSetCanFlyAckSequence;
        case CMSG_MOVE_SET_FACING:
            return MovementSetFacingSequence;
        case CMSG_MOVE_SET_PITCH:
            return MovementSetPitchSequence;
        case CMSG_MOVE_SET_RUN_MODE:
            return MovementSetRunModeSequence;
        case CMSG_MOVE_SET_WALK_MODE:
            return MovementSetWalkModeSequence;
        case CMSG_MOVE_SPLINE_DONE:
            return MovementSplineDoneSequence;
        case CMSG_MOVE_START_BACKWARD:
            return MovementStartBackwardSequence;
        case CMSG_MOVE_START_FORWARD:
            return MovementStartForwardSequence;
        case CMSG_MOVE_START_STRAFE_LEFT:
            return MovementStartStrafeLeftSequence;
        case CMSG_MOVE_START_STRAFE_RIGHT:
            return MovementStartStrafeRightSequence;
        case CMSG_MOVE_START_TURN_LEFT:
            return MovementStartTurnLeftSequence;
        case CMSG_MOVE_START_TURN_RIGHT:
            return MovementStartTurnRightSequence;
        case CMSG_MOVE_STOP:
            return MovementStopSequence;
        case CMSG_MOVE_STOP_STRAFE:
            return MovementStopStrafeSequence;
        case CMSG_MOVE_STOP_TURN:
            return MovementStopTurnSequence;
        case CMSG_MOVE_START_ASCEND:
            return MovementStartAscendSequence;
        case CMSG_MOVE_START_DESCEND:
            return MovementStartDescendSequence;
        case CMSG_MOVE_START_SWIM:
            return MovementStartSwimSequence;
        case CMSG_MOVE_STOP_SWIM:
            return MovementStopSwimSequence;
        case CMSG_MOVE_STOP_ASCEND:
            return MovementStopAscendSequence;
        case CMSG_MOVE_START_PITCH_DOWN:
            return MovementStartPitchDownSequence;
        case CMSG_MOVE_START_PITCH_UP:
            return MovementStartPitchUpSequence;
        case CMSG_MOVE_STOP_PITCH:
            return MovementStopPitchSequence;*/
        case MSG_MOVE_HEARTBEAT:
            return MovementHeartBeatSequence;
       /* case SMSG_PLAYER_MOVE:
            return PlayerMoveSequence;
        case CMSG_MOVE_KNOCK_BACK_ACK:
            return MoveKnockbackAckSequence;
        case SMSG_MOVE_UPDATE_KNOCK_BACK:
            return MoveUpdateKnockBackSequence;
        case CMSG_MOVE_NOT_ACTIVE_MOVER:
            return MoveNotActiveMoverSequence;*/
    }
    return NULL;
}