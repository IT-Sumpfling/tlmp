#pragma once

#include "DataTypes.h"

#include <map>
using std::map;

#define ENUMSTR(enumeration) "##enumeration##"

namespace TLMP {

  namespace Network {

    #define stringify(name) #name

    /** Version to signify any Messaging ID changes. */
    const u32 MessageVersion = 1;

    /** Network Control Messages */
    enum Message {

      S_VERSION = 0,                // These are expected to Match
      C_VERSION,                    // if not, both ends disconnect (Client will reply to server with version)

      C_REQUEST_HASGAMESTARTED,     // Requests the Server to determine if it's in the game yet,
      S_REPLY_HASGAMESTARTED,       // replies to client request
                                    // Clients are blocked from entering the game until the Server has established one

      S_PUSH_GAMESTARTED,           // Inform Clients that the game has started - Sent Post GameClient::LoadLevel
      S_PUSH_GAMEENDED,             // Inform Clients that the game has ended - Sent on TitleScreenProcess after leaving game (for now)

      C_PUSH_GAMEENTER,             // Inform Server that Client has entered the Game - Sent Post GameClient::LoadLevel
      C_PUSH_GAMEEXITED,            // Inform Server that Client has exited the Game - Sent on TitleScreenProcess after leaving game (for now)

      S_REQUEST_CHARINFO,           // Server Requests Client for Character Information (Name, Inventory, Class, Minions, etc.)
      C_REPLY_CHARINFO,             // Client Respone for Character Information Request
      S_REPLY_CHARID,               // Server Respone for setting Character Network ID

      C_PUSH_EQUIPMENT,             // Client Pushes it's Suppressed Player's Equipment to server
      S_REPLY_EQUIPMENT_ID,         // Server Respone to client equipment with the common ID

      S_PUSH_NEWCHAR,               // Server Pushes a New Character to the Client
      S_PUSH_NEWEQUIPMENT,          // Server Pushes a New Equipment

      S_PUSH_ADDCHARMINION,         // Server Pushes a Minion setup to Client
      S_PUSH_SETCHARALIGNMENT,      // Server Pushes a Character SetAlignment to Client

      C_PUSH_EQUIPMENT_DROP,        // Client Pushes an Equipment Drop from Character
      S_PUSH_EQUIPMENT_DROP,        // Server Pushes an Equipment Drop from Character
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_EQUIPMENT_PICKUP,      // Client Pushes an Equipment Pickup from Character
      S_PUSH_EQUIPMENT_PICKUP,      // Server Pushes an Equipment Pickup from Character
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_EQUIPMENT_EQUIP,       // Client Pushes an Equipment Equip from Character
      S_PUSH_EQUIPMENT_EQUIP,       // Server Pushes an Equipment Equip from Character
                                    // Note: These aren't requests, the Server will notify other clients of the action
     
      C_PUSH_EQUIPMENT_UNEQUIP,     // Client Pushes an Equipment Unequip from Character
      S_PUSH_EQUIPMENT_UNEQUIP,     // Server Pushes an Equipment Unequip from Character
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_EQUIPMENT_USE,         // Client Pushes an Equipment Use to Server
      S_PUSH_EQUIPMENT_USE,         // Server Pushes an Equipment Use to clients
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_EQUIPMENT_IDENTIFY,    // Client Pushes an Equipment Identify to Server
      S_PUSH_EQUIPMENT_IDENTIFY,    // Server Pushes an Equipment Identify to clients
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_EQUIPMENT_ADDENCHANT,  // Client Pushes an Equipment Add Enchant to Server
      S_PUSH_EQUIPMENT_ADDENCHANT,  // Server Pushes an Equipment Add Enchant to clients
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_EQUIPMENT_ADDSOCKET,   // Client Pushes an Equipment Add Socket to Server
      S_PUSH_EQUIPMENT_ADDSOCKET,   // Server Pushes an Equipment Add Socket to clients
                                    // Note: These aren't requests, the Server will notify other clients of the action

      C_PUSH_CHARACTER_SETDEST,     // Client Pushes a Character SetDestination to Server
      S_PUSH_CHARACTER_SETDEST,     // Server Pushes a Character SetDestination to Client

      C_PUSH_CHARACTER_ACTION,      // Client Pushes a Character Action
      S_PUSH_CHARACTER_ACTION,      // Server Pushes a Character Action

      C_PUSH_CHARACTER_ATTACK,      // Client Pushes a Character Attack
      S_PUSH_CHARACTER_ATTACK,      // Server Pushes a Character Attack

      C_PUSH_CHARACTER_USESKILL,    // Client Pushes a Character Use Skill
      S_PUSH_CHARACTER_USESKILL,    // Server Pushes a Character Use Skill
    };

    static const char* MessageString[] = {
      "S_VERSION",
      "C_VERSION",

      "C_REQUEST_HASGAMESTARTED",
      "S_REPLY_HASGAMESTARTED",

      "S_PUSH_GAMESTARTED",
      "S_PUSH_GAMEENDED",

      "C_PUSH_GAMEENTER",
      "C_PUSH_GAMEEXITED",

      "S_REQUEST_CHARINFO",
      "C_REPLY_CHARINFO",
      "S_REPLY_CHARID",

      "C_PUSH_EQUIPMENT",
      "S_REPLY_EQUIPMENT_ID",

      "S_PUSH_NEWCHAR",
      "S_PUSH_NEWEQUIPMENT",

      "S_PUSH_ADDCHARMINION",
      "S_PUSH_SETCHARALIGNMENT",

      "C_PUSH_EQUIPMENT_DROP",
      "S_PUSH_EQUIPMENT_DROP",
      
      "C_PUSH_EQUIPMENT_PICKUP",
      "S_PUSH_EQUIPMENT_PICKUP",

      "C_PUSH_EQUIPMENT_EQUIP",
      "S_PUSH_EQUIPMENT_EQUIP",
      
      "C_PUSH_EQUIPMENT_UNEQUIP",
      "S_PUSH_EQUIPMENT_UNEQUIP",
      
      "C_PUSH_EQUIPMENT_USE",
      "S_PUSH_EQUIPMENT_USE",
            
      "C_PUSH_EQUIPMENT_IDENTIFY",
      "S_PUSH_EQUIPMENT_IDENTIFY",
            
      "C_PUSH_EQUIPMENT_ADDENCHANT",
      "S_PUSH_EQUIPMENT_ADDENCHANT",
                  
      "C_PUSH_EQUIPMENT_ADDSOCKET",
      "S_PUSH_EQUIPMENT_ADDSOCKET",
                  
      "C_PUSH_CHARACTER_SETDEST",
      "S_PUSH_CHARACTER_SETDEST",

      "C_PUSH_CHARACTER_ACTION",
      "S_PUSH_CHARACTER_ACTION",
      
      "C_PUSH_CHARACTER_ATTACK",
      "S_PUSH_CHARACTER_ATTACK",
      
      "C_PUSH_CHARACTER_USESKILL",
      "S_PUSH_CHARACTER_USESKILL",
    };

  };

};