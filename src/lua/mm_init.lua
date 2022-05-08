-- Set LuaJIT properties
require('jit').opt.start('hotloop=28', 'maxmcode=2048', 'sizemcode=64', 'maxtrace=2000', 'maxrecord=8000')
-- ----------------------------------------------------------------------------------------------------
-- Load core functions that are needed by everything else
-- ----------------------------------------------------------------------------------------------------
-- Make sure core types are declared before anything else
local core = require('mm_core')
-- Declare core components
require('core_components')
-- Script API
local mm = require('mm_script_api')
-- ----------------------------------------------------------------------------------------------------
-- Declare global functions:
-- ----------------------------------------------------------------------------------------------------
-- Set engine settings
function set_config_value(key, value) -- Config is used internally
    core.config[key] = value
end
function set_attribute_value(key, subkey_or_value, value) -- Attrs are available to scripts
    if value then
        if mm.attrs[key] then
            mm.attrs[key][subkey_or_value] = value
        else
            mm.attrs[key] = {[subkey_or_value] = value}
        end
    else
        mm.attrs[key] = subkey_or_value
    end
end
-- Set the game state
function set_game_state(new_game_state)
    mm.game_state = new_game_state
end
-- Set the current time
function set_game_time(delta, absolute)
    mm.time.delta = delta
    mm.time.absolute = absolute
end
-- Create event handler function for scene events
local event_maps = require('mm_event_handlers')
handle_scene_events = event_maps.handle_scene_events
handle_game_events = event_maps.handle_game_events

-- Create message handler for ScriptedBehavior system
handle_messages = require('mm_scripted_behavior_handlers')