#include "modding.h"
#include "global.h"

#include "prevent_bss_reordering.h"
#include "z64message.h"
#include "global.h"

#include "gfxalloc.h"
#include "message_data_static.h"
#include "padmgr.h"
#include "sys_cmpdma.h"
#include "segment_symbols.h"
#include "attributes.h"

#include "z64actor.h"
#include "z64horse.h"
#include "z64shrink_window.h"
#include "z64save.h"

RECOMP_DECLARE_EVENT(mh_on_Message_Update(PlayState* play))

#define DEFINE_MSGMODE(name, _enumValue, _debugName) RECOMP_DECLARE_EVENT(mh_on_Message_Update_ ## name (PlayState* play))

#include "tables/msgmode_table.h"

#undef DEFINE_MSGMODE

RECOMP_DECLARE_EVENT(mh_on_Message_DrawMain(PlayState* play, Gfx** gfxP))

#define DEFINE_MSGMODE(name, _enumValue, _debugName) RECOMP_DECLARE_EVENT(mh_on_Message_DrawMain_ ## name (PlayState* play, Gfx** gfxP))

#include "tables/msgmode_table.h"

#undef DEFINE_MSGMODE

bool earlyReturn;

RECOMP_EXPORT void mh_set_return_flag(void) {
    earlyReturn = true;
}

RECOMP_EXPORT void mh_Message_Update_set_return_flag(void) {
    mh_set_return_flag();
}

RECOMP_EXPORT void mh_Message_DrawMain_set_return_flag(void) {
    mh_set_return_flag();
}

extern u8 D_801C6A70;

extern s32 sCharTexSize;
extern s32 sCharTexScale;
extern s32 D_801F6B08;

extern s16 sLastPlayedSong;

extern s16 sTextboxUpperYPositions[];
extern s16 sTextboxLowerYPositions[];
extern s16 sTextboxMidYPositions[];
extern s16 sTextboxXPositions[TEXTBOX_TYPE_MAX];

extern s16 D_801D0464[];
extern s16 D_801D045C[];

void func_80150A84(PlayState* play);
void Message_GrowTextbox(PlayState* play);
void Message_Decode(PlayState* play);
s32 Message_BombersNotebookProcessEventQueue(PlayState* play);
void Message_HandleChoiceSelection(PlayState* play, u8 numChoices);
bool Message_ShouldAdvanceSilent(PlayState* play);

void ShrinkWindow_Letterbox_SetSizeTarget(s32 target);
s32 ShrinkWindow_Letterbox_GetSizeTarget(void);

extern u8 sPlaybackState;
extern u16 sPlaybackStaffPos;
extern u8 sPlaybackStaffStopPos;

RECOMP_PATCH void Message_Update(PlayState* play) {
    static u8 D_801D0468 = 0;
    MessageContext* msgCtx = &play->msgCtx;
    SramContext* sramCtx = &play->sramCtx; // Optional
    PauseContext* pauseCtx = &play->pauseCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Input* input = CONTROLLER1(&play->state);
    s16 avgScreenPosY;
    s16 screenPosX;
    u16 temp_v1_2;
    s16 playerScreenPosY;
    s16 actorScreenPosY;
    s16 sp48;
    s32 sp44;
    s32 sp40;
    u16 sp3E;
    s16 var_v1;
    u16 temp;

    msgCtx->stickAdjX = input->rel.stick_x;
    msgCtx->stickAdjY = input->rel.stick_y;

    avgScreenPosY = 0;

    // If stickAdj is held, set a delay to allow the cursor to read the next input.
    // The first delay is given a longer time than all subsequent delays.
    if (msgCtx->stickAdjX < -30) {
        if (msgCtx->stickXRepeatState == -1) {
            msgCtx->stickXRepeatTimer--;
            if (msgCtx->stickXRepeatTimer < 0) {
                // Allow the input to register and apply the delay for all subsequent repeated inputs
                msgCtx->stickXRepeatTimer = 2;
            } else {
                // Cancel the current input
                msgCtx->stickAdjX = 0;
            }
        } else {
            // Allow the input to register and apply the delay for the first repeated input
            msgCtx->stickXRepeatTimer = 10;
            msgCtx->stickXRepeatState = -1;
        }
    } else if (msgCtx->stickAdjX > 30) {
        if (msgCtx->stickXRepeatState == 1) {
            msgCtx->stickXRepeatTimer--;
            if (msgCtx->stickXRepeatTimer < 0) {
                // Allow the input to register and apply the delay for all subsequent repeated inputs
                msgCtx->stickXRepeatTimer = 2;
            } else {
                // Cancel the current input
                msgCtx->stickAdjX = 0;
            }
        } else {
            // Allow the input to register and apply the delay for the first repeated input
            msgCtx->stickXRepeatTimer = 10;
            msgCtx->stickXRepeatState = 1;
        }
    } else {
        msgCtx->stickXRepeatState = 0;
    }

    if (msgCtx->stickAdjY < -30) {
        if (msgCtx->stickYRepeatState == -1) {
            msgCtx->stickYRepeatTimer--;
            if (msgCtx->stickYRepeatTimer < 0) {
                // Allow the input to register and apply the delay for all subsequent repeated inputs
                msgCtx->stickYRepeatTimer = 2;
            } else {
                // Cancel the current input
                msgCtx->stickAdjY = 0;
            }
        } else {
            // Allow the input to register and apply the delay for the first repeated input
            msgCtx->stickYRepeatTimer = 10;
            msgCtx->stickYRepeatState = -1;
        }
    } else if (msgCtx->stickAdjY > 30) {
        if (msgCtx->stickYRepeatState == 1) {
            msgCtx->stickYRepeatTimer--;
            if (msgCtx->stickYRepeatTimer < 0) {
                // Allow the input to register and apply the delay for all subsequent repeated inputs
                msgCtx->stickYRepeatTimer = 2;
            } else {
                // Cancel the current input
                msgCtx->stickAdjY = 0;
            }
        } else {
            // Allow the input to register and apply the delay for the first repeated input
            msgCtx->stickYRepeatTimer = 10;
            msgCtx->stickYRepeatState = 1;
        }
    } else {
        msgCtx->stickYRepeatState = 0;
    }

    if (msgCtx->msgLength == 0) {
        return;
    }

    // @mod Add hook to Message_Update.
    earlyReturn = false;

    mh_on_Message_Update(play);


#define DEFINE_MSGMODE(name, _enumValue, _debugName)    \
    case _enumValue:                                    \
        mh_on_Message_Update_ ## name(play);            \
        break;

    switch (msgCtx->msgMode) {
#include "tables/msgmode_table.h"
    }

#undef DEFINE_MSGMODE

    if (earlyReturn) {
        return;
    }

    switch (msgCtx->msgMode) {
        case MSGMODE_TEXT_START:
            D_801C6A70++;

            temp = false;
            if ((D_801C6A70 >= 4) || ((msgCtx->talkActor == NULL) && (D_801C6A70 >= 2))) {
                temp = true;
            }
            if (temp) {
                if (msgCtx->talkActor != NULL) {
                    Actor_GetScreenPos(play, &GET_PLAYER(play)->actor, &screenPosX, &playerScreenPosY);
                    Actor_GetScreenPos(play, msgCtx->talkActor, &screenPosX, &actorScreenPosY);
                    if (playerScreenPosY >= actorScreenPosY) {
                        avgScreenPosY = ((playerScreenPosY - actorScreenPosY) / 2) + actorScreenPosY;
                    } else {
                        avgScreenPosY = ((actorScreenPosY - playerScreenPosY) / 2) + playerScreenPosY;
                    }
                } else {
                    msgCtx->textboxX = msgCtx->textboxXTarget;
                    msgCtx->textboxY = msgCtx->textboxYTarget;
                }

                var_v1 = msgCtx->textBoxType;

                if ((u32)msgCtx->textBoxPos == 0) {
                    if ((play->sceneId == SCENE_UNSET_04) || (play->sceneId == SCENE_UNSET_05) ||
                        (play->sceneId == SCENE_UNSET_06)) {
                        if (avgScreenPosY < 100) {
                            msgCtx->textboxYTarget = sTextboxLowerYPositions[var_v1];
                        } else {
                            msgCtx->textboxYTarget = sTextboxUpperYPositions[var_v1];
                        }
                    } else {
                        if (avgScreenPosY < 160) {
                            msgCtx->textboxYTarget = sTextboxLowerYPositions[var_v1];
                        } else {
                            msgCtx->textboxYTarget = sTextboxUpperYPositions[var_v1];
                        }
                    }
                } else if (msgCtx->textBoxPos == 1) {
                    msgCtx->textboxYTarget = sTextboxUpperYPositions[var_v1];
                } else if (msgCtx->textBoxPos == 2) {
                    msgCtx->textboxYTarget = sTextboxMidYPositions[var_v1];
                } else if (msgCtx->textBoxPos == 7) {
                    msgCtx->textboxYTarget = 158;
                } else {
                    msgCtx->textboxYTarget = sTextboxLowerYPositions[var_v1];
                }

                msgCtx->textboxXTarget = sTextboxXPositions[var_v1];

                if ((gSaveContext.options.language == LANGUAGE_JPN) && !msgCtx->textIsCredits) {
                    msgCtx->unk11FFE[0] = (s16)(msgCtx->textboxYTarget + 7);
                    msgCtx->unk11FFE[1] = (s16)(msgCtx->textboxYTarget + 25);
                    msgCtx->unk11FFE[2] = (s16)(msgCtx->textboxYTarget + 43);
                } else {
                    msgCtx->unk11FFE[0] = (s16)(msgCtx->textboxYTarget + 20);
                    msgCtx->unk11FFE[1] = (s16)(msgCtx->textboxYTarget + 32);
                    msgCtx->unk11FFE[2] = (s16)(msgCtx->textboxYTarget + 44);
                }

                if ((msgCtx->textBoxType == TEXTBOX_TYPE_4) || (msgCtx->textBoxType == TEXTBOX_TYPE_5)) {
                    msgCtx->msgMode = MSGMODE_TEXT_STARTING;
                    msgCtx->textboxX = msgCtx->textboxXTarget;
                    msgCtx->textboxY = msgCtx->textboxYTarget;
                    msgCtx->unk12008 = 0x100;
                    msgCtx->unk1200A = 0x40;
                    msgCtx->unk1200C = 0x200;
                    msgCtx->unk1200E = 0x200;
                    break;
                }

                Message_GrowTextbox(play);
                Audio_PlaySfx_IfNotInCutscene(NA_SE_NONE);
                msgCtx->stateTimer = 0;
                msgCtx->msgMode = MSGMODE_TEXT_BOX_GROWING;

                if (!pauseCtx->itemDescriptionOn) {
                    func_80150A84(play);
                }
            }
            break;

        case MSGMODE_TEXT_BOX_GROWING:
            Message_GrowTextbox(play);
            break;

        case MSGMODE_TEXT_STARTING:
            msgCtx->msgMode = MSGMODE_TEXT_NEXT_MSG;
            if (!pauseCtx->itemDescriptionOn) {
                if (msgCtx->currentTextId == 0xFF) {
                    Interface_SetAButtonDoAction(play, DO_ACTION_STOP);
                } else if (msgCtx->currentTextId != 0xF8) {
                    Interface_SetAButtonDoAction(play, DO_ACTION_NEXT);
                }
            }
            break;

        case MSGMODE_TEXT_NEXT_MSG:
            Message_Decode(play);
            if (msgCtx->textFade) {
                Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
            }
            if (D_801D0468 != 0) {
                msgCtx->textDrawPos = msgCtx->decodedTextLen;
                D_801D0468 = 0;
            }
            break;

        case MSGMODE_TEXT_CONTINUING:
            msgCtx->stateTimer--;
            if (msgCtx->stateTimer == 0) {
                Message_Decode(play);
            }
            break;

        case MSGMODE_TEXT_DISPLAYING:
            if (msgCtx->textBoxType != TEXTBOX_TYPE_4) {
                if (CHECK_BTN_ALL(input->press.button, BTN_B) && !msgCtx->textUnskippable) {
                    msgCtx->textboxSkipped = true;
                    msgCtx->textDrawPos = msgCtx->decodedTextLen;
                } else if (CHECK_BTN_ALL(input->press.button, BTN_A) && !msgCtx->textUnskippable) {

                    while (true) {
                        temp_v1_2 = msgCtx->decodedBuffer.wchar[msgCtx->textDrawPos];
                        if ((temp_v1_2 == 0x10) || (temp_v1_2 == 0x12) || (temp_v1_2 == 0x1B) || (temp_v1_2 == 0x1C) ||
                            (temp_v1_2 == 0x1D) || (temp_v1_2 == 0x19) || (temp_v1_2 == 0xE0) || (temp_v1_2 == 0xBF) ||
                            (temp_v1_2 == 0x15) || (temp_v1_2 == 0x1A)) {
                            break;
                        }
                        msgCtx->textDrawPos++;
                    }
                }
            } else if (CHECK_BTN_ALL(input->press.button, BTN_A) && (msgCtx->textUnskippable == 0)) {
                while (true) {
                    temp_v1_2 = msgCtx->decodedBuffer.wchar[msgCtx->textDrawPos];
                    if ((temp_v1_2 == 0x10) || (temp_v1_2 == 0x12) || (temp_v1_2 == 0x1B) || (temp_v1_2 == 0x1C) ||
                        (temp_v1_2 == 0x1D) || (temp_v1_2 == 0x19) || (temp_v1_2 == 0xE0) || (temp_v1_2 == 0xBF) ||
                        (temp_v1_2 == 0x15) || (temp_v1_2 == 0x1A)) {
                        break;
                    }
                    msgCtx->textDrawPos++;
                }
            }
            break;

        case MSGMODE_TEXT_AWAIT_INPUT:
            if (Message_ShouldAdvance(play)) {
                msgCtx->msgMode = MSGMODE_TEXT_DISPLAYING;
                msgCtx->textDrawPos++;
            }
            break;

        case MSGMODE_TEXT_DELAYED_BREAK:
            msgCtx->stateTimer--;
            if (msgCtx->stateTimer == 0) {
                msgCtx->msgMode = MSGMODE_TEXT_NEXT_MSG;
            }
            break;

        case MSGMODE_TEXT_AWAIT_NEXT:
            if (Message_ShouldAdvance(play)) {
                msgCtx->msgMode = MSGMODE_TEXT_NEXT_MSG;
                msgCtx->msgBufPos++;
            }
            break;

        case MSGMODE_TEXT_DONE:
            if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_NORMAL) ||
                (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_SKIPPABLE)) {
                msgCtx->stateTimer--;
                if ((msgCtx->stateTimer == 0) ||
                    ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_SKIPPABLE) && Message_ShouldAdvance(play))) {
                    if (msgCtx->nextTextId != 0xFFFF) {
                        Audio_PlaySfx(NA_SE_SY_MESSAGE_PASS);
                        Message_ContinueTextbox(play, msgCtx->nextTextId);
                    } else if (msgCtx->bombersNotebookEventQueueCount != 0) {
                        if (Message_BombersNotebookProcessEventQueue(play) == 0) {
                            Message_CloseTextbox(play);
                        }
                    } else {
                        Message_CloseTextbox(play);
                    }
                }
            } else {
                if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_PERSISTENT) ||
                    (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_EVENT) ||
                    (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_EVENT2) ||
                    (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_PAUSE_MENU)) {
                    return;
                }

                switch (msgCtx->textboxEndType) {
                    case TEXTBOX_ENDTYPE_FADE_STAGES_1:
                        msgCtx->textColorAlpha += 20;
                        if (msgCtx->textColorAlpha >= 255) {
                            msgCtx->textColorAlpha = 255;
                            msgCtx->textboxEndType = TEXTBOX_ENDTYPE_FADE_STAGES_2;
                        }
                        break;

                    case TEXTBOX_ENDTYPE_FADE_STAGES_2:
                        msgCtx->stateTimer--;
                        if (msgCtx->stateTimer == 0) {
                            msgCtx->textboxEndType = TEXTBOX_ENDTYPE_FADE_STAGES_3;
                        }
                        break;

                    case TEXTBOX_ENDTYPE_FADE_STAGES_3:
                        msgCtx->textColorAlpha -= 20;
                        if (msgCtx->textColorAlpha <= 0) {
                            msgCtx->textColorAlpha = 0;
                            if (msgCtx->nextTextId != 0xFFFF) {
                                Audio_PlaySfx(NA_SE_SY_MESSAGE_PASS);
                                Message_ContinueTextbox(play, msgCtx->nextTextId);
                                return;
                            }
                            if (msgCtx->bombersNotebookEventQueueCount != 0) {
                                if (Message_BombersNotebookProcessEventQueue(play) == 0) {
                                    Message_CloseTextbox(play);
                                    return;
                                }
                            } else {
                                Message_CloseTextbox(play);
                                return;
                            }
                        }
                        break;

                    case TEXTBOX_ENDTYPE_TWO_CHOICE:
                        Message_HandleChoiceSelection(play, 1);
                        break;

                    case TEXTBOX_ENDTYPE_THREE_CHOICE:
                        Message_HandleChoiceSelection(play, 2);
                        break;

                    case TEXTBOX_ENDTYPE_12:
                        Message_HandleChoiceSelection(play, 1);
                        break;

                    default:
                        break;
                }

                if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) &&
                    (play->msgCtx.ocarinaMode == OCARINA_MODE_ACTIVE)) {
                    if (Message_ShouldAdvance(play)) {
                        if (msgCtx->choiceIndex == 0) {
                            play->msgCtx.ocarinaMode = OCARINA_MODE_WARP;
                        } else {
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        }
                        Message_CloseTextbox(play);
                    }
                } else if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) &&
                           (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_SOT)) {
                    if (Message_ShouldAdvance(play)) {
                        if (msgCtx->choiceIndex == 0) {
                            Audio_PlaySfx_MessageDecide();
                            msgCtx->msgMode = MSGMODE_NEW_CYCLE_0;
                            msgCtx->decodedTextLen -= 3;
                            msgCtx->unk120D6 = 0;
                            msgCtx->unk120D4 = 0;
                        } else {
                            Audio_PlaySfx_MessageCancel();
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                            Message_CloseTextbox(play);
                        }
                    }
                } else if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) &&
                           (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_INVERTED_TIME)) {
                    if (Message_ShouldAdvance(play)) {
                        if (msgCtx->choiceIndex == 0) {
                            Audio_PlaySfx_MessageDecide();
                            if (gSaveContext.save.timeSpeedOffset == 0) {
                                play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_INV_SOT_SLOW;
                                gSaveContext.save.timeSpeedOffset = -2;
                            } else {
                                play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_INV_SOT_FAST;
                                gSaveContext.save.timeSpeedOffset = 0;
                            }
                            Message_CloseTextbox(play);
                        } else {
                            Audio_PlaySfx_MessageCancel();
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                            Message_CloseTextbox(play);
                        }
                    }
                } else if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) &&
                           (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_DOUBLE_TIME)) {
                    if (Message_ShouldAdvance(play)) {
                        if (msgCtx->choiceIndex == 0) {
                            Audio_PlaySfx_MessageDecide();
                            if (gSaveContext.save.isNight != 0) {
                                gSaveContext.save.time = CLOCK_TIME(6, 0);
                            } else {
                                gSaveContext.save.time = CLOCK_TIME(18, 0);
                            }
                            play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_DOUBLE_SOT;
                            gSaveContext.timerStates[TIMER_ID_MOON_CRASH] = TIMER_STATE_OFF;
                        } else {
                            Audio_PlaySfx_MessageCancel();
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        }
                        Message_CloseTextbox(play);
                    }
                } else if ((msgCtx->textboxEndType != TEXTBOX_ENDTYPE_TWO_CHOICE) ||
                           (pauseCtx->state != PAUSE_STATE_OWL_WARP_CONFIRM)) {
                    if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) &&
                        (play->msgCtx.ocarinaMode == OCARINA_MODE_1B)) {
                        if (Message_ShouldAdvance(play)) {
                            if (msgCtx->choiceIndex == 0) {
                                Audio_PlaySfx_MessageDecide();
                                play->msgCtx.ocarinaMode = OCARINA_MODE_WARP_TO_ENTRANCE;
                            } else {
                                Audio_PlaySfx_MessageCancel();
                                play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                            }
                            Message_CloseTextbox(play);
                        }
                    } else if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_INPUT_BANK) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_INPUT_DOGGY_RACETRACK_BET) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_THREE_CHOICE) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_NORMAL) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_SKIPPABLE) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_STAGES_1) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_STAGES_2) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_STAGES_3) ||
                               (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_INPUT_BOMBER_CODE)) {
                        //! FAKE: debug?
                        if (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_FADE_NORMAL) {}
                    } else if (pauseCtx->itemDescriptionOn) {
                        if ((input->rel.stick_x != 0) || (input->rel.stick_y != 0) ||
                            CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_B) ||
                            CHECK_BTN_ALL(input->press.button, BTN_START)) {
                            Audio_PlaySfx(NA_SE_SY_DECIDE);
                            Message_CloseTextbox(play);
                        }
                    } else if (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_RESTRICTED_SONG) {
                        if (Message_ShouldAdvanceSilent(play)) {
                            Audio_PlaySfx(NA_SE_SY_DECIDE);
                            Message_CloseTextbox(play);
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        }
                    } else if ((msgCtx->currentTextId != 0x2790) && Message_ShouldAdvanceSilent(play)) {
                        if (msgCtx->nextTextId != 0xFFFF) {
                            Audio_PlaySfx(NA_SE_SY_MESSAGE_PASS);
                            Message_ContinueTextbox(play, msgCtx->nextTextId);
                        } else if ((msgCtx->bombersNotebookEventQueueCount == 0) ||
                                   (Message_BombersNotebookProcessEventQueue(play) != 1)) {
                            if (msgCtx->currentTextId == 0x579) {
                                gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
                            }
                            Audio_PlaySfx(NA_SE_SY_DECIDE);
                            Message_CloseTextbox(play);
                        }
                    }
                }
            }
            break;

        case MSGMODE_TEXT_CLOSING:
            msgCtx->stateTimer--;
            if (msgCtx->stateTimer != 0) {
                break;
            }

            if (sLastPlayedSong == OCARINA_SONG_SOARING) {
                if (interfaceCtx->restrictions.songOfSoaring == 0) {
                    if (Map_CurRoomHasMapI(play) || (play->sceneId == SCENE_SECOM)) {
                        Message_StartTextbox(play, 0x1B93, NULL);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_1B;
                        sLastPlayedSong = 0xFF;
                    } else if (!msgCtx->ocarinaSongEffectActive) {
                        if (gSaveContext.save.saveInfo.playerData.owlActivationFlags != 0) {
                            pauseCtx->unk_2C8 = pauseCtx->pageIndex;
                            pauseCtx->unk_2CA = pauseCtx->cursorPoint[4];
                            pauseCtx->pageIndex = PAUSE_ITEM;
                            pauseCtx->state = PAUSE_STATE_OWL_WARP_0;
                            func_800F4A10(play);
                            pauseCtx->pageIndex = PAUSE_MAP;
                            sLastPlayedSong = 0xFF;
                            Message_CloseTextbox(play);
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                            gSaveContext.prevHudVisibility = HUD_VISIBILITY_A_B;
                            Interface_SetBButtonInterfaceDoAction(play, DO_ACTION_STOP);
                            GameState_SetFramerateDivisor(&play->state, 2);
                            if (ShrinkWindow_Letterbox_GetSizeTarget() != 0) {
                                ShrinkWindow_Letterbox_SetSizeTarget(0);
                            }
                            Audio_PlaySfx_PauseMenuOpenOrClose(1);
                            break;
                        }
                        sLastPlayedSong = 0xFF;
                        Message_StartTextbox(play, 0xFB, NULL);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
                    } else {
                        msgCtx->stateTimer = 1;
                    }
                } else {
                    sLastPlayedSong = 0xFF;
                    Message_StartTextbox(play, 0x1B95, NULL);
                    play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
                }
                break;
            }

            if ((msgCtx->currentTextId == 0xC) || (msgCtx->currentTextId == 0xD) || (msgCtx->currentTextId == 0xC5) ||
                (msgCtx->currentTextId == 0xC6) || (msgCtx->currentTextId == 0xC7) ||
                (msgCtx->currentTextId == 0x2165) || (msgCtx->currentTextId == 0x2166) ||
                (msgCtx->currentTextId == 0x2167) || (msgCtx->currentTextId == 0x2168)) {
                gSaveContext.healthAccumulator = 20 * 0x10; // Refill 20 hearts
            }

            if ((play->csCtx.state == CS_STATE_IDLE) && (gSaveContext.save.cutsceneIndex < 0xFFF0) &&
                ((play->activeCamId == CAM_ID_MAIN) ||
                 ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF))) &&
                (play->msgCtx.ocarinaMode == OCARINA_MODE_END)) {
                if (((u32)gSaveContext.prevHudVisibility == HUD_VISIBILITY_IDLE) ||
                    (gSaveContext.prevHudVisibility == HUD_VISIBILITY_NONE) ||
                    (gSaveContext.prevHudVisibility == HUD_VISIBILITY_NONE_ALT)) {
                    gSaveContext.prevHudVisibility = HUD_VISIBILITY_ALL;
                }
                gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
            }

            if ((msgCtx->currentTextId >= 0x1BB2) && (msgCtx->currentTextId <= 0x1BB6) &&
                (play->actorCtx.flags & ACTORCTX_FLAG_TELESCOPE_ON)) {
                Message_StartTextbox(play, 0x5E6, NULL);
                break;
            }

            if (msgCtx->bombersNotebookEventQueueCount != 0) {
                if (Message_BombersNotebookProcessEventQueue(play) == 0) {
                    msgCtx->stateTimer = 1;
                }
                break;
            }

            msgCtx->msgLength = 0;
            msgCtx->msgMode = MSGMODE_NONE;
            msgCtx->currentTextId = 0;
            msgCtx->stateTimer = 0;
            XREG(31) = 0;

            if (pauseCtx->itemDescriptionOn) {
                Interface_SetAButtonDoAction(play, DO_ACTION_INFO);
                pauseCtx->itemDescriptionOn = false;
            }

            if (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_PERSISTENT) {
                msgCtx->textboxEndType = TEXTBOX_ENDTYPE_DEFAULT;
                play->msgCtx.ocarinaMode = OCARINA_MODE_WARP;
            } else {
                msgCtx->textboxEndType = TEXTBOX_ENDTYPE_DEFAULT;
            }

            if (EQ_MAX_QUEST_HEART_PIECE_COUNT) {
                RESET_HEART_PIECE_COUNT;
                gSaveContext.save.saveInfo.playerData.healthCapacity += 0x10;
                gSaveContext.save.saveInfo.playerData.health += 0x10;
            }

            if (msgCtx->ocarinaAction != OCARINA_ACTION_CHECK_NOTIME_DONE) {
                s16 pad;

                if (sLastPlayedSong == OCARINA_SONG_TIME) {
                    if (interfaceCtx->restrictions.songOfTime == 0) {
                        Message_StartTextbox(play, 0x1B8A, NULL);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_SOT;
                    } else {
                        sLastPlayedSong = 0xFF;
                        Message_StartTextbox(play, 0x1B95, NULL);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
                    }
                } else if (sLastPlayedSong == OCARINA_SONG_INVERTED_TIME) {
                    if (interfaceCtx->restrictions.invSongOfTime == 0) {
                        if (R_TIME_SPEED != 0) {
                            if (gSaveContext.save.timeSpeedOffset == 0) {
                                Message_StartTextbox(play, 0x1B8C, NULL);
                            } else {
                                Message_StartTextbox(play, 0x1B8D, NULL);
                            }
                            play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_INVERTED_TIME;
                        } else {
                            Message_StartTextbox(play, 0x1B8B, NULL);
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        }
                    } else {
                        sLastPlayedSong = 0xFF;
                        Message_StartTextbox(play, 0x1B95, NULL);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
                    }
                } else if (sLastPlayedSong == OCARINA_SONG_DOUBLE_TIME) {
                    if (interfaceCtx->restrictions.songOfDoubleTime == 0) {
                        if ((CURRENT_DAY != 3) || (gSaveContext.save.isNight == 0)) {
                            if (gSaveContext.save.isNight) {
                                Message_StartTextbox(play, D_801D0464[CURRENT_DAY - 1], NULL);
                            } else {
                                Message_StartTextbox(play, D_801D045C[CURRENT_DAY - 1], NULL);
                            }
                            play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_DOUBLE_TIME;
                        } else {
                            Message_StartTextbox(play, 0x1B94, NULL);
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        }
                    } else {
                        sLastPlayedSong = 0xFF;
                        Message_StartTextbox(play, 0x1B95, NULL);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
                    }
                } else if ((msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY_DONE) &&
                           ((play->msgCtx.ocarinaMode == OCARINA_MODE_ACTIVE) ||
                            (play->msgCtx.ocarinaMode == OCARINA_MODE_EVENT) ||
                            (play->msgCtx.ocarinaMode == OCARINA_MODE_PLAYED_SCARECROW_SPAWN) ||
                            (play->msgCtx.ocarinaMode == OCARINA_MODE_PLAYED_FULL_EVAN_SONG))) {
                    play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                    if (msgCtx->lastPlayedSong == OCARINA_SONG_SOARING) {
                        play->msgCtx.ocarinaMode = OCARINA_MODE_ACTIVE;
                    }
                }
                sLastPlayedSong = 0xFF;
            }
            break;

        case MSGMODE_OCARINA_PLAYING:
            if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                Message_CloseTextbox(play);
            } else {
                msgCtx->ocarinaButtonIndex = OCARINA_BTN_INVALID;
            }
            break;

        case MSGMODE_OCARINA_AWAIT_INPUT:
            if ((msgCtx->ocarinaAction != OCARINA_ACTION_PROMPT_EVAN_PART1_SECOND_HALF) &&
                (msgCtx->ocarinaAction != OCARINA_ACTION_PROMPT_EVAN_PART2_SECOND_HALF)) {
                if (Message_ShouldAdvance(play)) {
                    Message_DisplayOcarinaStaff(play, msgCtx->ocarinaAction);
                }
            }
            break;

        case MSGMODE_SCARECROW_SPAWN_RECORDING_ONGOING:
            if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
                AudioOcarina_SetRecordingState(OCARINA_RECORD_OFF);
                Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                Message_CloseTextbox(play);
                msgCtx->msgMode = MSGMODE_SCARECROW_SPAWN_RECORDING_FAILED;
            } else {
                msgCtx->ocarinaButtonIndex = OCARINA_BTN_INVALID;
            }
            break;

        case MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND:
            msgCtx->textboxColorAlphaCurrent += XREG(73);
            if (msgCtx->textboxColorAlphaCurrent >= 255) {
                msgCtx->textboxColorAlphaCurrent = 255;
                msgCtx->msgMode = MSGMODE_SCENE_TITLE_CARD_FADE_IN_TEXT;
            }
            break;

        case MSGMODE_SCENE_TITLE_CARD_FADE_IN_TEXT:
            msgCtx->textColorAlpha += XREG(73);
            if (msgCtx->textColorAlpha >= 255) {
                msgCtx->textColorAlpha = 255;
                msgCtx->msgMode = MSGMODE_SCENE_TITLE_CARD_DISPLAYING;
            }
            break;

        case MSGMODE_SCENE_TITLE_CARD_DISPLAYING:
            msgCtx->stateTimer--;
            if (msgCtx->stateTimer == 0) {
                msgCtx->msgMode = MSGMODE_SCENE_TITLE_CARD_FADE_OUT_TEXT;
            }
            break;

        case MSGMODE_SCENE_TITLE_CARD_FADE_OUT_TEXT:
            msgCtx->textColorAlpha -= XREG(70);
            if (msgCtx->textColorAlpha <= 0) {
                msgCtx->textColorAlpha = 0;
                msgCtx->msgMode = MSGMODE_SCENE_TITLE_CARD_FADE_OUT_BACKGROUND;
            }
            break;

        case MSGMODE_SCENE_TITLE_CARD_FADE_OUT_BACKGROUND:
            msgCtx->textboxColorAlphaCurrent -= XREG(70);
            if (msgCtx->textboxColorAlphaCurrent <= 0) {
                if ((msgCtx->currentTextId >= 0x1BB2) && (msgCtx->currentTextId <= 0x1BB6) &&
                    (play->actorCtx.flags & ACTORCTX_FLAG_TELESCOPE_ON)) {
                    Message_StartTextbox(play, 0x5E6, NULL);
                    Interface_SetHudVisibility(HUD_VISIBILITY_NONE_ALT);
                } else {
                    //! FAKE: debug?
                    if (msgCtx->currentTextId >= 0x100) {
                        if (msgCtx && msgCtx && msgCtx) {}
                    }
                    msgCtx->textboxColorAlphaCurrent = 0;
                    msgCtx->msgLength = 0;
                    msgCtx->msgMode = MSGMODE_NONE;
                    msgCtx->currentTextId = 0;
                    msgCtx->stateTimer = 0;
                }
            }
            break;

        case MSGMODE_NEW_CYCLE_0:
            play->state.unk_A3 = 1;
            sp44 = gSaveContext.save.cutsceneIndex;
            sp3E = CURRENT_TIME;
            sp40 = gSaveContext.save.day;

            Sram_SaveEndOfCycle(play);
            gSaveContext.timerStates[TIMER_ID_MOON_CRASH] = TIMER_STATE_OFF;
            func_8014546C(&play->sramCtx);

            gSaveContext.save.day = sp40;
            gSaveContext.save.time = sp3E;
            gSaveContext.save.cutsceneIndex = sp44;

            if (gSaveContext.fileNum != 0xFF) {
                Sram_SetFlashPagesDefault(&play->sramCtx, gFlashSaveStartPages[gSaveContext.fileNum * 2],
                                          gFlashSpecialSaveNumPages[gSaveContext.fileNum * 2]);
                Sram_StartWriteToFlashDefault(&play->sramCtx);
            }
            msgCtx->msgMode = MSGMODE_NEW_CYCLE_1;
            break;

        case MSGMODE_NEW_CYCLE_1:
            if (gSaveContext.fileNum != 0xFF) {
                play->state.unk_A3 = 1;
                if (play->sramCtx.status == 0) {
                    play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_SOT;
                    msgCtx->msgMode = MSGMODE_NEW_CYCLE_2;
                }
            } else {
                play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_SOT;
                msgCtx->msgMode = MSGMODE_NEW_CYCLE_2;
            }
            break;

        case MSGMODE_OWL_SAVE_0:
            play->state.unk_A3 = 1;
            gSaveContext.save.isOwlSave = true;
            Play_SaveCycleSceneFlags(play);
            func_8014546C(&play->sramCtx);

            if (gSaveContext.fileNum != 0xFF) {
                Sram_SetFlashPagesOwlSave(&play->sramCtx, gFlashOwlSaveStartPages[gSaveContext.fileNum * 2],
                                          gFlashOwlSaveNumPages[gSaveContext.fileNum * 2]);
                Sram_StartWriteToFlashOwlSave(&play->sramCtx);
            }
            msgCtx->msgMode = MSGMODE_OWL_SAVE_1;
            break;

        case MSGMODE_OWL_SAVE_1:
            if (gSaveContext.fileNum != 0xFF) {
                play->state.unk_A3 = 1;
                if (play->sramCtx.status == 0) {
                    play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_SOT;
                    msgCtx->msgMode = MSGMODE_OWL_SAVE_2;
                }
            } else {
                play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_SOT;
                msgCtx->msgMode = MSGMODE_OWL_SAVE_2;
            }

            if (msgCtx->msgMode == MSGMODE_OWL_SAVE_2) {
                gSaveContext.gameMode = GAMEMODE_OWL_SAVE;
                play->transitionTrigger = TRANS_TRIGGER_START;
                play->transitionType = TRANS_TYPE_FADE_BLACK;
                play->nextEntrance = ENTRANCE(CUTSCENE, 0);
                gSaveContext.save.cutsceneIndex = 0;
                gSaveContext.sceneLayer = 0;
            }
            break;

        case MSGMODE_9:
        case MSGMODE_PAUSED:
        case MSGMODE_NEW_CYCLE_2:
        case MSGMODE_OWL_SAVE_2:
            break;

        default:
            msgCtx->ocarinaButtonIndex = OCARINA_BTN_INVALID;
            break;
    }
}

extern s16 D_801CFC78[TEXTBOX_TYPE_MAX];
extern s16 D_801D03A8[TEXTBOX_TYPE_MAX];
extern s16 sOcarinaButtonIndexBufPos;
extern s16 sOcarinaButtonFlashColorIndex;
extern s16 sOcarinaButtonFlashTimer;
extern u8 sPlayerFormOcarinaInstruments[];
extern u8 sOcarinaButtonIndexBuf[12];
extern s16 sOcarinaButtonIndexBufLen;
extern s16 D_801C6A94;
extern s8 sOcarinaSongFanfareIoData[PLAYER_FORM_MAX];
extern s16 sOcarinaSongFanfares[];

void Message_SetView(View* view);
void Message_DrawText(PlayState* play, Gfx** gfxP);
void Message_DrawTextBox(PlayState* play, Gfx** gfxP);
void Message_DrawTextboxIcon(PlayState* play, Gfx** gfxP, s16 x, s16 y);
void Message_ResetOcarinaButtonState(PlayState* play);
void Message_DrawOcarinaButtons(PlayState* play, Gfx** gfxP);
void Message_DrawSceneTitleCard(PlayState* play, Gfx** gfxP);
void func_80148CBC(PlayState* play, Gfx** gfxP, u8 arg2);
void func_801496C8(PlayState* play);
void func_80149454(PlayState* play);
void func_801491DC(PlayState* play);
void func_80149048(PlayState* play);
void func_80148558(PlayState* play, Gfx** gfxP, s16 arg2, s16 arg3);
void func_80148D64(PlayState* play);
void func_80147F18(PlayState* play, Gfx** gfxP, s16 arg2, s16 arg3);
void Message_ResetOcarinaButtonAlphas(void);
void func_80152CAC(PlayState* play);
void Message_SpawnSongEffect(PlayState* play);
void Message_FlashOcarinaButtons(void);

RECOMP_PATCH void Message_DrawMain(PlayState* play, Gfx** gfxP) {
    s32 pad;
    MessageContext* msgCtx = &play->msgCtx;
    Gfx* gfx;
    u16 i;
    u16 buttonIndexPos;
    u8 ocarinaError;
    s32 j;
    s16 temp_v0_33;
    s16 temp;

    gfx = *gfxP;

    gSPSegment(gfx++, 0x02, play->interfaceCtx.parameterSegment);
    gSPSegment(gfx++, 0x07, msgCtx->textboxSegment);

    if (msgCtx->msgLength != 0) {
        if (!msgCtx->textIsCredits) {
            Message_SetView(&msgCtx->view);
            Gfx_SetupDL39_Ptr(&gfx);
            if (msgCtx->ocarinaAction != OCARINA_ACTION_37) {
                if ((msgCtx->msgMode != MSGMODE_18) && (msgCtx->msgMode != MSGMODE_39) &&
                    (msgCtx->msgMode != MSGMODE_3C) && (msgCtx->msgMode != MSGMODE_3F) &&
                    (msgCtx->msgMode != MSGMODE_3A) && (msgCtx->msgMode != MSGMODE_3D) &&
                    (msgCtx->msgMode != MSGMODE_40) &&
                    (((msgCtx->msgMode >= MSGMODE_TEXT_BOX_GROWING) && (msgCtx->msgMode <= MSGMODE_TEXT_DONE)) ||
                     ((msgCtx->msgMode >= MSGMODE_NEW_CYCLE_0) && (msgCtx->msgMode <= MSGMODE_OWL_SAVE_2))) &&
                    (D_801CFC78[msgCtx->textBoxType] != 0xE)) {
                    Message_DrawTextBox(play, &gfx);
                }
            }
        }
        Gfx_SetupDL39_Ptr(&gfx);
        gDPSetAlphaCompare(gfx++, G_AC_NONE);
        gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE,
                          0);

        if (YREG(0) != msgCtx->msgMode) {
            YREG(0) = msgCtx->msgMode;
            YREG(1) = msgCtx->ocarinaAction;
        }

        // @mod Add hook to Message_DrawMain.
        earlyReturn = false;

        mh_on_Message_DrawMain(play, gfxP);

#define DEFINE_MSGMODE(name, _enumValue, _debugName)    \
    case _enumValue:                                    \
        mh_on_Message_DrawMain_ ## name(play, gfxP);    \
        break;

        switch (msgCtx->msgMode) {
#include "tables/msgmode_table.h"
        }

#undef DEFINE_MGSMODE

        if (earlyReturn) {
            return;
        }

        switch (msgCtx->msgMode) {
            case MSGMODE_TEXT_START:
            case MSGMODE_TEXT_BOX_GROWING:
            case MSGMODE_TEXT_STARTING:
            case MSGMODE_TEXT_NEXT_MSG:
                break;

            case MSGMODE_TEXT_CONTINUING:
                if (msgCtx->stateTimer == 1) {
                    for (i = 0, j = 0; i < 48; i++, j += FONT_CHAR_TEX_SIZE) {
                        // Font_LoadChar(play, '　', j);
                    }
                    Message_DrawText(play, &gfx);
                } else {
                    gDPPipeSync(gfx++);
                    gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
                    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, 0);
                    gDPSetEnvColor(gfx++, 0, 0, 0, 255);
                }
                break;

            case MSGMODE_TEXT_DISPLAYING:
            case MSGMODE_TEXT_DELAYED_BREAK:
            case MSGMODE_9:
                if ((gSaveContext.options.language == LANGUAGE_JPN) && !msgCtx->textIsCredits) {
                    if (msgCtx->textDelay != 0) {
                        msgCtx->textDrawPos += msgCtx->textDelay;
                    }
                    Message_DrawTextNES(play, &gfx, 0);
                    if (msgCtx->msgMode == MSGMODE_TEXT_DISPLAYING) {
                        Message_DrawTextNES(play, &gfx, (s32)msgCtx->textDrawPos);
                    }
                } else if (msgCtx->textIsCredits) {
                    Message_DrawTextCredits(play, &gfx);
                } else {
                    if (msgCtx->textDelay != 0) {
                        msgCtx->textDrawPos += msgCtx->textDelay;
                    }
                    Message_DrawTextNES(play, &gfx, 0);
                    if (msgCtx->msgMode == MSGMODE_TEXT_DISPLAYING) {
                        Message_DrawTextNES(play, &gfx, (s32)msgCtx->textDrawPos);
                    }
                }
                break;

            case MSGMODE_NEW_CYCLE_0:
            case MSGMODE_NEW_CYCLE_1:
            case MSGMODE_NEW_CYCLE_2:
            case MSGMODE_OWL_SAVE_0:
            case MSGMODE_OWL_SAVE_1:
            case MSGMODE_OWL_SAVE_2:
                Message_DrawTextNES(play, &gfx, 0);
                break;

            case MSGMODE_TEXT_AWAIT_INPUT:
            case MSGMODE_TEXT_AWAIT_NEXT:
                Message_DrawText(play, &gfx);
                Message_DrawTextboxIcon(play, &gfx, 158,
                                        (s16)(D_801D03A8[msgCtx->textBoxType] + msgCtx->textboxYTarget));
                break;

            case MSGMODE_OCARINA_STARTING:
            case MSGMODE_SONG_DEMONSTRATION_STARTING:
            case MSGMODE_SONG_PROMPT_STARTING:
            case MSGMODE_32:
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();
                play->msgCtx.ocarinaMode = OCARINA_MODE_ACTIVE;
                if ((msgCtx->ocarinaAction != OCARINA_ACTION_PROMPT_EVAN_PART1_SECOND_HALF) &&
                    (msgCtx->ocarinaAction != OCARINA_ACTION_PROMPT_EVAN_PART2_SECOND_HALF)) {
                    msgCtx->ocarinaStaff->pos = 0;
                    sOcarinaButtonIndexBufPos = 0;
                    Message_ResetOcarinaButtonState(play);
                }
                sOcarinaButtonFlashColorIndex = 1;
                sOcarinaButtonFlashTimer = 3;
                if (msgCtx->msgMode == MSGMODE_OCARINA_STARTING) {
                    if ((msgCtx->ocarinaAction == OCARINA_ACTION_0) ||
                        (msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY) ||
                        (msgCtx->ocarinaAction == OCARINA_ACTION_SCARECROW_SPAWN_RECORDING) ||
                        (msgCtx->ocarinaAction == OCARINA_ACTION_CHECK_NOTIME) ||
                        (msgCtx->ocarinaAction >= OCARINA_ACTION_CHECK_TIME)) {
                        if ((msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY) ||
                            (msgCtx->ocarinaAction == OCARINA_ACTION_CHECK_NOTIME)) {
                            if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_52_10)) {
                                AudioOcarina_StartDefault(msgCtx->ocarinaAvailableSongs | 0xC0000000);
                            } else if (CHECK_EVENTINF(EVENTINF_31)) {
                                AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                                AudioOcarina_StartDefault(0x80800000);
                            } else {
                                AudioOcarina_StartAtSongStartingPos((msgCtx->ocarinaAvailableSongs + 0x80000) |
                                                                    0xC0000000);
                                AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                            }
                        } else {
                            AudioOcarina_StartDefault(msgCtx->ocarinaAvailableSongs);
                        }
                    } else {
                        AudioOcarina_StartDefault((1 << msgCtx->ocarinaAction) | 0x80000000);
                    }
                    msgCtx->msgMode = MSGMODE_OCARINA_PLAYING;
                    if (CHECK_EVENTINF(EVENTINF_24)) {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEKU_TRUMPET);
                    } else if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_41_20)) {
                        AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                        //! FAKE:
                        (void)CUR_FORM;
                        if (gSaveContext.save.playerForm == 4) {}
                    } else {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_AMPLIFIED_GUITAR);
                    }
                } else if (msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION_STARTING) {
                    msgCtx->stateTimer = 20;
                    msgCtx->msgMode = MSGMODE_19;
                } else if (msgCtx->msgMode == MSGMODE_32) {
                    AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                    AudioOcarina_StartDefault(0x80800000);
                    msgCtx->msgMode = MSGMODE_33;
                } else { // MSGMODE_SONG_PROMPT_STARTING
                    if (CHECK_EVENTINF(EVENTINF_24)) {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEKU_TRUMPET);
                    } else {
                        AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                    }

                    if ((msgCtx->ocarinaAction == OCARINA_ACTION_PROMPT_EVAN_PART1_SECOND_HALF) ||
                        (msgCtx->ocarinaAction == OCARINA_ACTION_PROMPT_EVAN_PART2_SECOND_HALF)) {
                        AudioOcarina_StartForSongCheck(
                            (1 << ((msgCtx->ocarinaAction - OCARINA_ACTION_PROMPT_EVAN_PART1_SECOND_HALF) +
                                   OCARINA_SONG_EVAN_PART1)) |
                                0x80000000,
                            4);
                        msgCtx->msgMode = MSGMODE_SONG_PROMPT;
                    } else {
                        if ((msgCtx->ocarinaAction >= OCARINA_ACTION_PROMPT_WIND_FISH_HUMAN) &&
                            (msgCtx->ocarinaAction <= OCARINA_ACTION_PROMPT_WIND_FISH_DEKU)) {
                            AudioOcarina_StartDefault(
                                (1 << ((msgCtx->ocarinaAction - OCARINA_ACTION_PROMPT_WIND_FISH_HUMAN) +
                                       OCARINA_SONG_WIND_FISH_HUMAN)) |
                                0x80000000);
                        } else {
                            AudioOcarina_StartDefault(
                                (1 << ((msgCtx->ocarinaAction - OCARINA_ACTION_PROMPT_SONATA) + OCARINA_SONG_SONATA)) |
                                0x80000000);
                        }
                        msgCtx->msgMode = MSGMODE_SONG_PROMPT;
                    }
                }

                if ((msgCtx->ocarinaAction != OCARINA_ACTION_FREE_PLAY) &&
                    (msgCtx->ocarinaAction != OCARINA_ACTION_CHECK_NOTIME)) {
                    Message_DrawText(play, &gfx);
                }
                break;

            case MSGMODE_OCARINA_PLAYING:
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();

                if ((u32)msgCtx->ocarinaStaff->pos != 0) {
                    if ((msgCtx->ocarinaStaff->pos == 1) && (sOcarinaButtonIndexBufPos == 8)) {
                        sOcarinaButtonIndexBufPos = 0;
                    }

                    if (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1)) {
                        msgCtx->ocarinaButtonIndex = sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos - 1] =
                            msgCtx->ocarinaStaff->buttonIndex;
                        sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos] = OCARINA_BTN_INVALID;
                        sOcarinaButtonIndexBufPos++;
                    }
                }

                msgCtx->songPlayed = msgCtx->ocarinaStaff->state;

                if (msgCtx->ocarinaStaff->state <= OCARINA_SONG_SCARECROW_SPAWN) {
                    if (msgCtx->ocarinaStaff->state == OCARINA_SONG_EVAN_PART1) {
                        AudioOcarina_ResetAndReadInput();
                        AudioOcarina_StartDefault(0x80100000);
                    } else if (msgCtx->ocarinaStaff->state == OCARINA_SONG_EVAN_PART2) {
                        Audio_PlaySfx(NA_SE_SY_CORRECT_CHIME);
                        AudioOcarina_SetOcarinaDisableTimer(0, 20);
                        Message_CloseTextbox(play);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_PLAYED_FULL_EVAN_SONG;
                    } else if ((msgCtx->ocarinaStaff->state == OCARINA_SONG_SCARECROW_SPAWN) ||
                               (msgCtx->ocarinaStaff->state == OCARINA_SONG_INVERTED_TIME) ||
                               (msgCtx->ocarinaStaff->state == OCARINA_SONG_DOUBLE_TIME) ||
                               (msgCtx->ocarinaStaff->state == OCARINA_SONG_GORON_LULLABY_INTRO) ||
                               CHECK_QUEST_ITEM(QUEST_SONG_SONATA + msgCtx->ocarinaStaff->state)) {
                        sLastPlayedSong = msgCtx->ocarinaStaff->state;
                        msgCtx->lastPlayedSong = msgCtx->ocarinaStaff->state;
                        msgCtx->songPlayed = msgCtx->ocarinaStaff->state;
                        msgCtx->msgMode = MSGMODE_E;
                        msgCtx->stateTimer = 20;

                        if (msgCtx->ocarinaAction == OCARINA_ACTION_CHECK_NOTIME) {
                            if ((msgCtx->ocarinaStaff->state <= OCARINA_SONG_SARIAS) ||
                                (msgCtx->ocarinaStaff->state == OCARINA_SONG_SCARECROW_SPAWN)) {
                                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                                Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                                msgCtx->msgMode = MSGMODE_OCARINA_STARTING;
                            } else {
                                Message_ContinueTextbox(play, 0x1B5B);
                                msgCtx->msgMode = MSGMODE_SONG_PLAYED;
                                msgCtx->textBoxType = TEXTBOX_TYPE_3;
                                msgCtx->stateTimer = 10;
                                Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                                Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
                            }
                        } else if (msgCtx->ocarinaAction == OCARINA_ACTION_CHECK_SCARECROW_SPAWN) {
                            if (msgCtx->ocarinaStaff->state <= OCARINA_SONG_STORMS) {
                                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                                Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                                msgCtx->stateTimer = 10;
                                msgCtx->msgMode = MSGMODE_OCARINA_FAIL;
                            } else {
                                Message_ContinueTextbox(play, 0x1B5B);
                                msgCtx->msgMode = MSGMODE_SONG_PLAYED;
                                msgCtx->textBoxType = TEXTBOX_TYPE_3;
                                msgCtx->stateTimer = 10;
                                Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                                Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
                            }
                        } else if (msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY) {
                            Message_ContinueTextbox(play, 0x1B5B);
                            msgCtx->msgMode = MSGMODE_SONG_PLAYED;
                            msgCtx->textBoxType = TEXTBOX_TYPE_3;
                            msgCtx->stateTimer = 10;
                            Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                        } else {
                            Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                        }
                        Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
                    } else {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                        Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                        msgCtx->msgMode = MSGMODE_OCARINA_STARTING;
                    }
                } else if (msgCtx->ocarinaStaff->state == OCARINA_SONG_TERMINA_WALL) {
                    Message_CloseTextbox(play);
                    play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                    SET_EVENTINF(EVENTINF_32);
                    Audio_PlaySfx(NA_SE_SY_CORRECT_CHIME);
                    AudioOcarina_SetOcarinaDisableTimer(0, 20);
                } else if (msgCtx->ocarinaStaff->state == 0xFF) {
                    if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_52_10)) {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                        Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                        msgCtx->stateTimer = 10;
                        msgCtx->msgMode = MSGMODE_OCARINA_FAIL;
                    } else {
                        AudioOcarina_SetSongStartingPos();
                        AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                        AudioOcarina_StartAtSongStartingPos((msgCtx->ocarinaAvailableSongs + 0x80000) | 0xC0000000);
                    }
                }
                if ((msgCtx->ocarinaAction != OCARINA_ACTION_FREE_PLAY) &&
                    (msgCtx->ocarinaAction != OCARINA_ACTION_CHECK_NOTIME)) {
                    Message_DrawText(play, &gfx);
                }
                break;

            case MSGMODE_E:
            case MSGMODE_SONG_PROMPT_SUCCESS:
            case MSGMODE_22:
            case MSGMODE_SCARECROW_SPAWN_RECORDING_DONE:
            case MSGMODE_34:
                Message_FlashOcarinaButtons();
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);

                    if (msgCtx->msgMode == MSGMODE_E) {
                        Message_ContinueTextbox(play, 0x1B5B);
                        msgCtx->msgMode = MSGMODE_SONG_PLAYED;
                        msgCtx->textBoxType = TEXTBOX_TYPE_3;
                        msgCtx->stateTimer = 1;
                    } else if (msgCtx->msgMode == MSGMODE_SONG_PROMPT_SUCCESS) {
                        Message_ContinueTextbox(play, 0x1B5B);
                        msgCtx->msgMode = MSGMODE_SONG_PLAYED;
                        msgCtx->textBoxType = TEXTBOX_TYPE_3;
                        msgCtx->stateTimer = 1;
                    } else if (msgCtx->msgMode == MSGMODE_22) {
                        msgCtx->msgMode = MSGMODE_23;
                        play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                        msgCtx->textBoxType = TEXTBOX_TYPE_0;
                    } else if (msgCtx->msgMode == MSGMODE_34) {
                        if (msgCtx->songPlayed == OCARINA_SONG_TERMINA_WALL) {
                            Message_CloseTextbox(play);
                            play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                            Audio_PlaySfx(NA_SE_SY_CORRECT_CHIME);
                        } else {
                            Message_CloseTextbox(play);
                            play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        }
                    } else {
                        Message_CloseTextbox(play);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                    }
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_OCARINA_FAIL:
            case MSGMODE_SONG_PROMPT_FAIL:
                Message_DrawText(play, &gfx);
                FALLTHROUGH;
            case MSGMODE_OCARINA_FAIL_NO_TEXT:
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    D_801C6A94 = 1;
                    if (msgCtx->msgMode == MSGMODE_SONG_PROMPT_FAIL) {
                        Message_ContinueTextbox(play, 0x1B89);
                        Message_Decode(play);
                        msgCtx->msgMode = MSGMODE_SONG_PROMPT_NOTES_DROP;
                    } else {
                        msgCtx->msgMode = MSGMODE_OCARINA_NOTES_DROP;
                    }
                }
                break;

            case MSGMODE_OCARINA_NOTES_DROP:
            case MSGMODE_SONG_PROMPT_NOTES_DROP:
                for (i = 0; i < 5; i++) {
                    msgCtx->ocarinaButtonsPosY[i] += D_801C6A94;
                }
                D_801C6A94 += D_801C6A94;
                if (D_801C6A94 >= 0x226) {
                    Message_ResetOcarinaButtonAlphas();
                    if (msgCtx->msgMode == MSGMODE_SONG_PROMPT_NOTES_DROP) {
                        msgCtx->msgMode = MSGMODE_OCARINA_AWAIT_INPUT;
                        msgCtx->stateTimer = 10;
                    } else {
                        msgCtx->msgMode = MSGMODE_OCARINA_STARTING;
                    }
                    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                }
                break;

            case MSGMODE_SONG_PLAYED:
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                    Message_Decode(play);
                    msgCtx->msgMode = MSGMODE_SETUP_DISPLAY_SONG_PLAYED;
                    msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();
                    msgCtx->ocarinaStaff->pos = 0;
                    sOcarinaButtonIndexBufPos = 0;
                    Message_ResetOcarinaButtonState(play);
                    Message_SpawnSongEffect(play);
                }
                break;

            case MSGMODE_SETUP_DISPLAY_SONG_PLAYED:
                Message_DrawText(play, &gfx);
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);

                if ((msgCtx->ocarinaAction >= OCARINA_ACTION_PROMPT_WIND_FISH_HUMAN) &&
                    (msgCtx->ocarinaAction <= OCARINA_ACTION_PROMPT_WIND_FISH_DEKU)) {
                    AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                    AudioOcarina_SetPlaybackSong(msgCtx->ocarinaAction - OCARINA_ACTION_SCARECROW_LONG_RECORDING, 1);
                } else {
                    AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                    AudioOcarina_SetPlaybackSong((u8)msgCtx->songPlayed + 1, 1);
                    if (msgCtx->songPlayed != OCARINA_SONG_SCARECROW_SPAWN) {
                        Audio_PlayFanfareWithPlayerIOPort7((u16)sOcarinaSongFanfares[msgCtx->songPlayed],
                                                           (u8)sOcarinaSongFanfareIoData[CUR_FORM]);
                        AudioSfx_MuteBanks(0x20);
                    }
                }
                play->msgCtx.ocarinaMode = OCARINA_MODE_ACTIVE;
                if (msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY) {
                    msgCtx->ocarinaAction = OCARINA_ACTION_FREE_PLAY_DONE;
                }
                if (msgCtx->ocarinaAction == OCARINA_ACTION_CHECK_NOTIME) {
                    msgCtx->ocarinaAction = OCARINA_ACTION_CHECK_NOTIME_DONE;
                }
                sOcarinaButtonIndexBufPos = 0;
                msgCtx->msgMode = MSGMODE_DISPLAY_SONG_PLAYED;
                break;

            case MSGMODE_DISPLAY_SONG_PLAYED_TEXT_BEGIN:
                if (msgCtx->songPlayed == OCARINA_SONG_SCARECROW_SPAWN) {
                    Message_ContinueTextbox(play, 0x1B6B);
                } else {
                    Message_ContinueTextbox(play, 0x1B72 + msgCtx->songPlayed);
                }
                Message_Decode(play);
                msgCtx->msgMode = MSGMODE_16;
                msgCtx->stateTimer = 20;
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_16:
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    msgCtx->msgMode = MSGMODE_17;
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_17:
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                Message_ResetOcarinaButtonState(play);
                msgCtx->msgMode = MSGMODE_18;
                msgCtx->stateTimer = 2;
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_18:
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    Message_CloseTextbox(play);
                    if (msgCtx->songPlayed == OCARINA_SONG_EPONAS) {
                        gHorsePlayedEponasSong = true;
                    }

                    if (msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY_DONE) {
                        if (sLastPlayedSong == OCARINA_SONG_ELEGY) {
                            if ((play->sceneId == SCENE_F40) || (play->sceneId == SCENE_F41) ||
                                (play->sceneId == SCENE_IKANAMAE) || (play->sceneId == SCENE_CASTLE) ||
                                (play->sceneId == SCENE_IKNINSIDE) || (play->sceneId == SCENE_IKANA) ||
                                (play->sceneId == SCENE_INISIE_N) || (play->sceneId == SCENE_INISIE_R) ||
                                (play->sceneId == SCENE_INISIE_BS) || (play->sceneId == SCENE_RANDOM) ||
                                (play->sceneId == SCENE_REDEAD) || (play->sceneId == SCENE_TOUGITES)) {
                                play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                            } else {
                                sLastPlayedSong = 0xFF;
                                Message_StartTextbox(play, 0x1B95, NULL);
                                play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
                            }
                        } else {
                            play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                            if (msgCtx->songPlayed == OCARINA_SONG_SCARECROW_SPAWN) {
                                play->msgCtx.ocarinaMode = OCARINA_MODE_PLAYED_SCARECROW_SPAWN;
                            }
                        }
                    } else if (msgCtx->ocarinaAction >= OCARINA_ACTION_CHECK_SONATA) {
                        if ((OCARINA_ACTION_CHECK_SONATA + msgCtx->songPlayed) == msgCtx->ocarinaAction) {
                            play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                        } else {
                            play->msgCtx.ocarinaMode = msgCtx->songPlayed - 1;
                        }
                    } else if ((OCARINA_ACTION_PROMPT_SONATA + msgCtx->songPlayed) == msgCtx->ocarinaAction) {
                        play->msgCtx.ocarinaMode = OCARINA_MODE_EVENT;
                    } else {
                        play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                    }
                }
                break;

            case MSGMODE_19:
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    func_80152CAC(play);
                    sOcarinaButtonIndexBufPos = 0;
                    msgCtx->msgMode = MSGMODE_SONG_DEMONSTRATION;
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_DISPLAY_SONG_PLAYED:
            case MSGMODE_SONG_DEMONSTRATION:
                msgCtx->ocarinaStaff = AudioOcarina_GetPlaybackStaff();
                if (msgCtx->ocarinaStaff->state == 0) {
                    if ((msgCtx->ocarinaAction == OCARINA_ACTION_DEMONSTRATE_EVAN_PART1_SECOND_HALF) ||
                        (msgCtx->ocarinaAction == OCARINA_ACTION_DEMONSTRATE_EVAN_PART2_SECOND_HALF)) {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                    }
                    if ((msgCtx->ocarinaAction >= OCARINA_ACTION_PROMPT_WIND_FISH_HUMAN) &&
                        (msgCtx->ocarinaAction <= OCARINA_ACTION_PROMPT_WIND_FISH_DEKU)) {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                        Message_CloseTextbox(play);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                    } else if (msgCtx->msgMode == MSGMODE_DISPLAY_SONG_PLAYED) {
                        msgCtx->msgMode = MSGMODE_DISPLAY_SONG_PLAYED_TEXT_BEGIN;
                    } else {
                        msgCtx->msgMode = MSGMODE_SONG_DEMONSTRATION_DONE;
                    }
                } else {
                    if ((sOcarinaButtonIndexBufPos != 0) && (msgCtx->ocarinaStaff->pos == 1)) {
                        sOcarinaButtonIndexBufPos = 0;
                    }

                    if (((u32)msgCtx->ocarinaStaff->pos != 0) &&
                        (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1))) {
                        msgCtx->ocarinaButtonIndex = sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos - 1] =
                            msgCtx->ocarinaStaff->buttonIndex;
                        sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos] = OCARINA_BTN_INVALID;
                        sOcarinaButtonIndexBufPos++;
                    }
                }
                FALLTHROUGH;
            case MSGMODE_SONG_DEMONSTRATION_DONE:
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_SONG_PROMPT:
            case MSGMODE_33:
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();

                if (((u32)msgCtx->ocarinaStaff->pos != 0) &&
                    (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1))) {
                    msgCtx->ocarinaButtonIndex = sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos - 1] =
                        msgCtx->ocarinaStaff->buttonIndex;
                    sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos] = OCARINA_BTN_INVALID;
                    sOcarinaButtonIndexBufPos++;
                }

                if (msgCtx->ocarinaStaff->state <= OCARINA_SONG_SCARECROW_SPAWN) {
                    if ((msgCtx->ocarinaAction == OCARINA_ACTION_PROMPT_EVAN_PART1_SECOND_HALF) ||
                        (msgCtx->ocarinaAction == OCARINA_ACTION_PROMPT_EVAN_PART2_SECOND_HALF)) {
                        msgCtx->songPlayed = msgCtx->ocarinaStaff->state;
                        msgCtx->msgMode = MSGMODE_22;
                    } else {
                        msgCtx->songPlayed = msgCtx->ocarinaStaff->state;
                        msgCtx->msgMode = MSGMODE_SONG_PROMPT_SUCCESS;
                        if (msgCtx->ocarinaStaff->state == OCARINA_SONG_GORON_LULLABY_INTRO) {
                            Item_Give(play, ITEM_SONG_LULLABY_INTRO);
                        } else if (!((msgCtx->ocarinaAction >= OCARINA_ACTION_PROMPT_WIND_FISH_HUMAN) &&
                                     (msgCtx->ocarinaAction <= OCARINA_ACTION_PROMPT_WIND_FISH_DEKU)) &&
                                   (msgCtx->ocarinaStaff->state != OCARINA_SONG_NEW_WAVE)) {
                            Item_Give(play,
                                      (gOcarinaSongItemMap[msgCtx->ocarinaStaff->state] + ITEM_SONG_SONATA) & 0xFF);
                        }
                    }
                    msgCtx->stateTimer = 20;
                    Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                } else if (msgCtx->ocarinaStaff->state == OCARINA_SONG_TERMINA_WALL) {
                    msgCtx->songPlayed = msgCtx->ocarinaStaff->state;
                    msgCtx->msgMode = MSGMODE_34;
                    Item_Give(play, (gOcarinaSongItemMap[msgCtx->ocarinaStaff->state] + ITEM_SONG_SONATA) & 0xFF);
                    msgCtx->stateTimer = 20;
                    AudioOcarina_SetOcarinaDisableTimer(0, 20);
                    Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                } else if (msgCtx->ocarinaStaff->state == 0xFF) {
                    Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                    msgCtx->stateTimer = 10;
                    if (msgCtx->msgMode == MSGMODE_SONG_PROMPT) {
                        msgCtx->msgMode = MSGMODE_SONG_PROMPT_FAIL;
                    } else {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                        Message_CloseTextbox(play);
                    }
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_OCARINA_AWAIT_INPUT:
                Message_DrawText(play, &gfx);
                if ((msgCtx->ocarinaAction == OCARINA_ACTION_PROMPT_EVAN_PART1_SECOND_HALF) ||
                    (msgCtx->ocarinaAction == OCARINA_ACTION_PROMPT_EVAN_PART2_SECOND_HALF)) {
                    msgCtx->stateTimer--;
                    if (msgCtx->stateTimer == 0) {
                        msgCtx->msgMode = MSGMODE_21;
                        msgCtx->textBoxType = TEXTBOX_TYPE_0;
                    }
                }
                break;

            case MSGMODE_SCARECROW_LONG_RECORDING_START:
                AudioOcarina_SetRecordingState(OCARINA_RECORD_SCARECROW_LONG);
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);
                msgCtx->ocarinaStaff = AudioOcarina_GetRecordingStaff();
                msgCtx->ocarinaStaff->pos = 0;
                sOcarinaButtonIndexBufPos = 0;
                sOcarinaButtonIndexBufLen = 0;
                Message_ResetOcarinaButtonState(play);
                msgCtx->msgMode = MSGMODE_SCARECROW_LONG_RECORDING_ONGOING;
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_SCARECROW_LONG_RECORDING_ONGOING:
                msgCtx->ocarinaStaff = AudioOcarina_GetRecordingStaff();

                if (((u32)msgCtx->ocarinaStaff->pos != 0) &&
                    (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1))) {

                    if (sOcarinaButtonIndexBufLen >= 8) {
                        for (buttonIndexPos = sOcarinaButtonIndexBufLen - 8, i = 0; i < 8; i++, buttonIndexPos++) {
                            sOcarinaButtonIndexBuf[buttonIndexPos] = sOcarinaButtonIndexBuf[buttonIndexPos + 1];
                        }
                        sOcarinaButtonIndexBufLen--;
                    }

                    msgCtx->ocarinaButtonIndex = sOcarinaButtonIndexBuf[sOcarinaButtonIndexBufLen] =
                        msgCtx->ocarinaStaff->buttonIndex;
                    sOcarinaButtonIndexBufLen++;
                    sOcarinaButtonIndexBuf[sOcarinaButtonIndexBufLen] = OCARINA_BTN_INVALID;
                    sOcarinaButtonIndexBufPos++;
                    if (msgCtx->ocarinaStaff) {}
                    if (msgCtx->ocarinaStaff->pos == 8) {
                        sOcarinaButtonIndexBufPos = 0;
                    }
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_SCARECROW_LONG_DEMONSTRATION:
            case MSGMODE_SCARECROW_SPAWN_DEMONSTRATION:
                msgCtx->ocarinaStaff = AudioOcarina_GetPlaybackStaff();

                if (((u32)msgCtx->ocarinaStaff->pos != 0) &&
                    (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1))) {
                    if (sOcarinaButtonIndexBufLen >= 8) {
                        for (buttonIndexPos = sOcarinaButtonIndexBufLen - 8, i = 0; i < 8; i++, buttonIndexPos++) {
                            sOcarinaButtonIndexBuf[buttonIndexPos] = sOcarinaButtonIndexBuf[buttonIndexPos + 1];
                        }
                        sOcarinaButtonIndexBufLen--;
                    }
                    sOcarinaButtonIndexBuf[sOcarinaButtonIndexBufLen] = msgCtx->ocarinaStaff->buttonIndex;
                    sOcarinaButtonIndexBufLen++;
                    sOcarinaButtonIndexBuf[sOcarinaButtonIndexBufLen] = OCARINA_BTN_INVALID;
                    sOcarinaButtonIndexBufPos++;
                    if (msgCtx->ocarinaStaff->pos == 8) {
                        sOcarinaButtonIndexBufLen = sOcarinaButtonIndexBufPos = 0;
                    }
                }

                if (msgCtx->stateTimer == 0) {
                    if (msgCtx->ocarinaStaff->state == 0) {
                        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                        play->msgCtx.ocarinaMode = OCARINA_MODE_11;
                        Message_CloseTextbox(play);
                    }
                } else {
                    msgCtx->stateTimer--;
                }
                break;

            case MSGMODE_SCARECROW_SPAWN_RECORDING_START:
                AudioOcarina_SetRecordingState(OCARINA_RECORD_SCARECROW_SPAWN);
                AudioOcarina_SetInstrument(sPlayerFormOcarinaInstruments[CUR_FORM]);
                msgCtx->msgMode = MSGMODE_SCARECROW_SPAWN_RECORDING_ONGOING;
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_SCARECROW_SPAWN_RECORDING_ONGOING:
                msgCtx->ocarinaStaff = AudioOcarina_GetRecordingStaff();
                if ((u32)msgCtx->ocarinaStaff->pos != 0) {
                    if (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1)) {
                        msgCtx->ocarinaButtonIndex = sOcarinaButtonIndexBuf[sOcarinaButtonIndexBufPos] =
                            msgCtx->ocarinaStaff->buttonIndex;
                        sOcarinaButtonIndexBufPos++;
                        sOcarinaButtonIndexBuf[sOcarinaButtonIndexBufPos] = OCARINA_BTN_INVALID;
                    }
                }

                if (msgCtx->ocarinaStaff->state == 0) {
                    msgCtx->stateTimer = 20;
                    gSaveContext.save.saveInfo.scarecrowSpawnSongSet = true;
                    msgCtx->msgMode = MSGMODE_SCARECROW_SPAWN_RECORDING_DONE;
                    Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                    Lib_MemCpy(gSaveContext.save.saveInfo.scarecrowSpawnSong, gScarecrowSpawnSongPtr,
                               sizeof(gSaveContext.save.saveInfo.scarecrowSpawnSong));
                    for (i = 0; i < ARRAY_COUNT(gSaveContext.save.saveInfo.scarecrowSpawnSong); i++) {
                        // osSyncPrintf("%d, ", gSaveContext.scarecrowSpawnSong[i]);
                    }
                } else if (msgCtx->ocarinaStaff->state == 0xFF) {
                    AudioOcarina_SetRecordingState(OCARINA_RECORD_OFF);
                    Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                    Message_CloseTextbox(play);
                    msgCtx->msgMode = MSGMODE_SCARECROW_SPAWN_RECORDING_FAILED;
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_SCARECROW_SPAWN_RECORDING_FAILED:
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
                Message_StartTextbox(play, 0x1647, NULL);
                play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                break;

            case MSGMODE_2F:
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();
                msgCtx->ocarinaStaff->pos = 0;
                sOcarinaButtonIndexBufPos = 0;
                play->msgCtx.ocarinaMode = OCARINA_MODE_ACTIVE;
                Message_ResetOcarinaButtonState(play);
                AudioOcarina_StartDefault(msgCtx->ocarinaAvailableSongs | 0xC0000000);
                msgCtx->msgMode = MSGMODE_30;
                break;

            case MSGMODE_30:
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();
                if (((u32)msgCtx->ocarinaStaff->pos != 0) &&
                    (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1))) {
                    msgCtx->ocarinaButtonIndex = msgCtx->ocarinaStaff->buttonIndex;
                    msgCtx->ocarinaStaff->pos = 0;
                    sOcarinaButtonIndexBufPos = 0;
                    Message_ResetOcarinaButtonState(play);
                    msgCtx->msgMode = MSGMODE_31;
                }
                break;

            case MSGMODE_35:
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);
                AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_DEFAULT);
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();
                msgCtx->ocarinaStaff->pos = 0;
                sOcarinaButtonIndexBufPos = 0;
                play->msgCtx.ocarinaMode = OCARINA_MODE_ACTIVE;
                Message_ResetOcarinaButtonState(play);
                sOcarinaButtonFlashColorIndex = 1;
                sOcarinaButtonFlashTimer = 3;
                AudioOcarina_StartWithSongNoteLengths(
                    (1 << ((msgCtx->ocarinaAction - OCARINA_ACTION_TIMED_PROMPT_SONATA) & 0xFFFF)) | 0x80000000);
                msgCtx->msgMode = MSGMODE_36;
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_36:
                msgCtx->ocarinaStaff = AudioOcarina_GetPlayingStaff();

                if ((u32)msgCtx->ocarinaStaff->pos != 0) {
                    if (sOcarinaButtonIndexBufPos == (msgCtx->ocarinaStaff->pos - 1)) {
                        sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos - 1] = msgCtx->ocarinaStaff->buttonIndex;
                        sOcarinaButtonIndexBuf[msgCtx->ocarinaStaff->pos] = OCARINA_BTN_INVALID;
                        sOcarinaButtonIndexBufPos++;
                    }
                }

                if (msgCtx->ocarinaStaff->state <= OCARINA_SONG_SCARECROW_SPAWN) {
                    msgCtx->songPlayed = msgCtx->ocarinaStaff->state;
                    msgCtx->msgMode = MSGMODE_37;
                    Item_Give(play, (gOcarinaSongItemMap[msgCtx->ocarinaStaff->state] + ITEM_SONG_SONATA) & 0xFF);
                    msgCtx->stateTimer = 20;
                    Audio_PlaySfx(NA_SE_SY_TRE_BOX_APPEAR);
                } else if (msgCtx->ocarinaStaff->state == 0xFF) {
                    ocarinaError = func_8019B5AC();
                    if (ocarinaError == OCARINA_ERROR_2) {
                        Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                        D_801C6A94 = 1;
                        msgCtx->msgMode = MSGMODE_3B;
                    } else if (ocarinaError == OCARINA_ERROR_3) {
                        Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                        D_801C6A94 = 1;
                        msgCtx->msgMode = MSGMODE_3E;
                    } else {
                        Audio_PlaySfx(NA_SE_SY_OCARINA_ERROR);
                        D_801C6A94 = 1;
                        msgCtx->msgMode = MSGMODE_38;
                    }
                }
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_37:
                Message_FlashOcarinaButtons();
                Message_DrawText(play, &gfx);
                break;

            case MSGMODE_38:
            case MSGMODE_3B:
            case MSGMODE_3E:
                // notes drop
                for (i = 0; i < 5; i++) {
                    msgCtx->ocarinaButtonsPosY[i] += D_801C6A94;
                }

                D_801C6A94 += D_801C6A94;
                if (D_801C6A94 >= 0x226) {
                    Message_ResetOcarinaButtonAlphas();
                    msgCtx->textBoxType = TEXTBOX_TYPE_0;
                    msgCtx->textboxColorRed = msgCtx->textboxColorGreen = msgCtx->textboxColorBlue = 0;
                    msgCtx->stateTimer = 3;
                    msgCtx->msgMode++;
                }
                break;

            case MSGMODE_39:
            case MSGMODE_3C:
            case MSGMODE_3F:
                msgCtx->stateTimer--;
                if (msgCtx->stateTimer == 0) {
                    msgCtx->msgMode++;
                }
                break;

            case MSGMODE_TEXT_DONE:
                switch (msgCtx->textboxEndType) {
                    case TEXTBOX_ENDTYPE_INPUT_BANK:
                        temp_v0_33 = msgCtx->unk120BE;
                        temp = msgCtx->unk11FFA + (msgCtx->unk11FFC * temp_v0_33);
                        func_80147F18(play, &gfx,
                                      msgCtx->unk11F1A[temp_v0_33] +
                                          (s32)(16.0f * msgCtx->textCharScale * (msgCtx->unk120C2 + 5)) - 1,
                                      temp);
                        func_80148D64(play);
                        break;

                    case TEXTBOX_ENDTYPE_INPUT_DOGGY_RACETRACK_BET:
                        temp_v0_33 = msgCtx->unk120BE;
                        temp = msgCtx->unk11FFA + (msgCtx->unk11FFC * temp_v0_33);
                        func_80148558(play, &gfx,
                                      msgCtx->unk11F1A[temp_v0_33] + (s32)(16.0f * msgCtx->textCharScale * 5.0f) - 1,
                                      temp);
                        func_80149048(play);
                        break;

                    case TEXTBOX_ENDTYPE_INPUT_BOMBER_CODE:
                        temp_v0_33 = msgCtx->unk120BE;
                        temp = msgCtx->unk11FFA + (msgCtx->unk11FFC * temp_v0_33);
                        func_80147F18(play, &gfx,
                                      msgCtx->unk11F1A[temp_v0_33] +
                                          (s32)(16.0f * msgCtx->textCharScale * (msgCtx->unk120C2 + 5)) - 1,
                                      temp);
                        func_801491DC(play);
                        break;

                    case TEXTBOX_ENDTYPE_INPUT_LOTTERY_CODE:
                        temp_v0_33 = msgCtx->unk120BE;
                        temp = msgCtx->unk11FFA + (msgCtx->unk11FFC * temp_v0_33);
                        func_80147F18(play, &gfx,
                                      msgCtx->unk11F1A[temp_v0_33] +
                                          (s32)(16.0f * msgCtx->textCharScale * (msgCtx->unk120C2 + 5)) - 1,
                                      temp);
                        func_80149454(play);
                        break;

                    case TEXTBOX_ENDTYPE_64:
                        temp_v0_33 = msgCtx->unk120BE;
                        temp = msgCtx->unk11FFA + (msgCtx->unk11FFC * temp_v0_33);
                        func_80147F18(play, &gfx,
                                      msgCtx->unk11F1A[temp_v0_33] +
                                          (s32)(16.0f * msgCtx->textCharScale * (msgCtx->unk120C2 + 4)) - 6,
                                      temp);
                        func_801496C8(play);
                        break;

                    default:
                        break;
                }

                gDPPipeSync(gfx++);
                gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                                  PRIMITIVE, 0);
                gDPSetEnvColor(gfx++, 0, 0, 0, 255);
                Message_DrawText(play, &gfx);

                switch (msgCtx->textboxEndType) {
                    case TEXTBOX_ENDTYPE_TWO_CHOICE:
                        func_80148CBC(play, &gfx, 1);
                        break;

                    case TEXTBOX_ENDTYPE_THREE_CHOICE:
                        func_80148CBC(play, &gfx, 2);
                        break;

                    case TEXTBOX_ENDTYPE_12:
                        func_80148CBC(play, &gfx, 1);
                        break;

                    case TEXTBOX_ENDTYPE_PERSISTENT:
                    case TEXTBOX_ENDTYPE_PAUSE_MENU:
                    case TEXTBOX_ENDTYPE_FADE_NORMAL:
                    case TEXTBOX_ENDTYPE_FADE_SKIPPABLE:
                    case TEXTBOX_ENDTYPE_FADE_STAGES_1:
                    case TEXTBOX_ENDTYPE_FADE_STAGES_2:
                    case TEXTBOX_ENDTYPE_FADE_STAGES_3:
                    case TEXTBOX_ENDTYPE_INPUT_BOMBER_CODE:
                        break;

                    case TEXTBOX_ENDTYPE_EVENT:
                    case TEXTBOX_ENDTYPE_INPUT_BANK:
                    case TEXTBOX_ENDTYPE_INPUT_DOGGY_RACETRACK_BET:
                    default:
                        Message_DrawTextboxIcon(play, &gfx, 158,
                                                (s16)(D_801D03A8[msgCtx->textBoxType] + msgCtx->textboxYTarget));
                        break;

                    case TEXTBOX_ENDTYPE_EVENT2:
                        Message_DrawTextboxIcon(play, &gfx, 158,
                                                (s16)(D_801D03A8[msgCtx->textBoxType] + msgCtx->textboxYTarget));
                        break;
                }
                break;

            case MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND:
            case MSGMODE_SCENE_TITLE_CARD_FADE_IN_TEXT:
            case MSGMODE_SCENE_TITLE_CARD_DISPLAYING:
            case MSGMODE_SCENE_TITLE_CARD_FADE_OUT_TEXT:
            case MSGMODE_SCENE_TITLE_CARD_FADE_OUT_BACKGROUND:
                Message_DrawSceneTitleCard(play, &gfx);
                break;

            case MSGMODE_21:
            case MSGMODE_23:
            case MSGMODE_31:
            case MSGMODE_3A:
            case MSGMODE_3D:
            case MSGMODE_40:
            case MSGMODE_TEXT_CLOSING:
            case MSGMODE_PAUSED:
                break;

            case MSGMODE_24:
            case MSGMODE_25:
            case MSGMODE_26:
            default:
                msgCtx->msgMode = MSGMODE_TEXT_DISPLAYING;
                break;
        }
    }
    Message_DrawOcarinaButtons(play, &gfx);
    *gfxP = gfx;
}