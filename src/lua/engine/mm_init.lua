-- ----------------------------------------------------------------------------------------------------
-- Load core functions that are needed by everything else
-- ----------------------------------------------------------------------------------------------------
-- Make sure core types are declared before anything else
require('mm_core')
-- Declare core components
require('core_components')

-- ----------------------------------------------------------------------------------------------------
-- Declare global functions:
-- ----------------------------------------------------------------------------------------------------
-- Create event handler function for scene events
local event_maps = require('mm_event_handlers')
handle_scene_events = event_maps.handle_scene_events
handle_game_events = event_maps.handle_game_events

-- Create message handler for ScriptedBehavior system
handle_messages = require('mm_scripted_behavior_handlers')
