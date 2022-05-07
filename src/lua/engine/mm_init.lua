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
handle_scene_events=require('SceneEvents')
-- Create message handler for ScriptedBehavior system
handle_messages=require('ScriptedBehaviorSystem')
