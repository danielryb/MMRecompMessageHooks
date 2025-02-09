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

RECOMP_DECLARE_EVENT(on_Message_Update(PlayState* play))

bool early_return;

RECOMP_EXPORT void set_return_flag(void) {
    early_return = true;
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
    early_return = false;

    on_Message_Update(play);

    if (early_return) {
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