#include "Client.h"

using namespace TLMP::Network;

Client& Client::getSingleton()
{
  static Client m_pSingleton;
  return m_pSingleton;
}

Client::Client()
{
  m_pClient = NULL;
  m_pBitStream = new RakNet::BitStream(1024);

  Reset();
}

Client::~Client()
{
  delete m_pBitStream;
}

void Client::Reset()
{
  m_bWaitingForGame = false;
  m_bGameStarted = false;
  m_bServerGameStarted = false;
  m_bSuppressNetwork_SetDestination = false;
  m_bSuppressNetwork_CharacterCreation = true;
  m_bSuppressNetwork_EquipmentCreation = true;
  m_bSuppressNetwork_EquipmentDrop = false;
  m_bSuppressNetwork_EquipmentPickup = true;
  m_bSuppressNetwork_SendEquipmentUnequip = false;
  m_bSuppressNetwork_CharacterAction = true;
  m_bSuppressNetwork_CharacterAttack = true;
  m_bSuppressNetwork_SendUseSkill = false;
  m_bSuppressNetwork_SendEquipmentStack = false;

  m_bIsSendingPickup = false;
  m_bIsSendingUseSkill = false;
  m_bIsEquipmentAddingGem = false;
}

void Client::Connect(const char* address, u16 port)
{
  log("Client connecting to %s (%i)", address, port);

  if (m_pClient) {
    m_pClient->Shutdown(0);
  }

  m_pClient = RakNetworkFactory::GetRakPeerInterface();

  // Debugging - allow 2 minutes until disconnect
  //m_pClient->SetTimeoutTime(120000, UNASSIGNED_SYSTEM_ADDRESS);
  //--

  SocketDescriptor socketDescriptor(0, 0);
  m_pClient->Startup(1, 30, &socketDescriptor, 1);
  m_pClient->Connect(address, port, 0, 0);
}

void Client::Disconnect()
{
  multiplayerLogger.WriteLine(Info, L"Client disconnecting...");
  log(L"Client disconnecting...");

  if (m_pClient) {
    Reset();

    m_pClient->Shutdown(0);
    m_pClient = NULL;
  }
}

void Client::SetCallback_OnConnected(OnConnected callback)
{
  m_pOnConnected.push_back(callback);
}

void Client::SetCallback_OnDisconnected(OnDisconnected callback)
{
  m_pOnDisconnected.push_back(callback);
}

void Client::SetCallback_OnConnectFailed(OnConnectFailed callback)
{
  m_pOnConnectFailed.push_back(callback);
}

void Client::ReceiveMessages()
{
  if (!m_pClient)
    return;

  Packet *packet = m_pClient->Receive();
  while (packet)
  {
    switch(packet->data[0])
    {
    case ID_CONNECTION_REQUEST_ACCEPTED:
      {
        multiplayerLogger.WriteLine(Info, L"Client connected to server.");
        logColor(B_GREEN, L"Client connected to server.");

        vector<OnConnected>::iterator itr;
        for (itr = m_pOnConnected.begin(); itr != m_pOnConnected.end(); itr++) {
          SystemAddress *sysAddress = new SystemAddress();
          *sysAddress = packet->systemAddress;
          (*itr)((void*)sysAddress);
        }
      }

      break;
    case ID_NO_FREE_INCOMING_CONNECTIONS:
      {
        multiplayerLogger.WriteLine(Info, L"Could not connect to server: No Room.");
        logColor(B_GREEN, L"Could not connect to server: No Room.");

        vector<OnDisconnected>::iterator itr;
        for (itr = m_pOnConnectFailed.begin(); itr != m_pOnConnectFailed.end(); itr++) {
          (*itr)(NULL);
        }
      }
      break;
    case ID_DISCONNECTION_NOTIFICATION:
      {
        multiplayerLogger.WriteLine(Info, L"Client disconnected.");
        logColor(B_GREEN, L"Client disconnected.");

        vector<OnDisconnected>::iterator itr;
        for (itr = m_pOnDisconnected.begin(); itr != m_pOnDisconnected.end(); itr++) {
          (*itr)(NULL);
        }
      }
      break;
    case ID_CONNECTION_LOST:
      {
        multiplayerLogger.WriteLine(Info, L"Client connection lost.");
        logColor(B_GREEN, L"Client connection lost.");

        vector<OnDisconnected>::iterator itr;
        for (itr = m_pOnDisconnected.begin(); itr != m_pOnDisconnected.end(); itr++) {
          (*itr)(NULL);
        }
      }
      break;
    case ID_CONNECTION_ATTEMPT_FAILED:
      {
        multiplayerLogger.WriteLine(Info, L"Client connection failed.");
        logColor(B_GREEN, L"Client connection failed.");

        vector<OnDisconnected>::iterator itr;
        for (itr = m_pOnConnectFailed.begin(); itr != m_pOnConnectFailed.end(); itr++) {
          (*itr)(NULL);
        }
      }
      break;
    case ID_USER_PACKET_ENUM+1:
      u32 msg;
      m_pBitStream->Reset();
      m_pBitStream->Write((const char *)packet->data, packet->length);
      m_pBitStream->IgnoreBits(8);
      m_pBitStream->Read<u32>(msg);

      WorkMessage((Message)msg, m_pBitStream);

      break;
    }

    m_pClient->DeallocatePacket(packet);
    packet = m_pClient->Receive();
  }
}

void Client::OnConnect(void *args)
{
  /*
  TLMP::NetworkMessages::Player player;

  // Init player
  player.set_id((u64)15);
  player.set_name("drivehappy");
  player.set_type(NetworkMessages::Player_ClassType_ALCHEMIST);

  SendMessage<NetworkMessages::Player>(C_PLAYER_INFO, &player);
  */

  multiplayerLogger.WriteLine(Info, L"Client Connected.");
  log("Client Connected.");
}

void Client::WorkMessage(Message msg, RakNet::BitStream *bitStream)
{
  wstring msgString = convertAsciiToWide(MessageString[msg]);

  // For sake of less spam
  if (msg != S_PUSH_CHARACTER_SETDEST) {
    multiplayerLogger.WriteLine(Info, L"Client Received Message: %s", msgString.c_str());
    logColor(B_GREEN, L"Client Received Message: %s", msgString.c_str());
  }

  switch (msg) {
  case S_VERSION:
    {
      NetworkMessages::Version *msgVersion = ParseMessage<NetworkMessages::Version>(m_pBitStream);
      u32 version = (u32)(msgVersion->version());

      HandleVersion(version);
    }
    break;

  case S_REPLY_HASGAMESTARTED:
    {
      NetworkMessages::GameHasStarted *msgGameHasStarted = ParseMessage<NetworkMessages::GameHasStarted>(m_pBitStream);
      bool gameStarted = msgGameHasStarted->started();

      HandleHasGameStarted(gameStarted);
    }
    break;

  case S_PUSH_GAMESTARTED:
    {
      NetworkMessages::GameStarted *msgGameStarted = ParseMessage<NetworkMessages::GameStarted>(m_pBitStream);

      HandleGameStarted();
    }
    break;

  case S_PUSH_GAMEENDED:
    {
      NetworkMessages::GameEnded *msgGameEnded = ParseMessage<NetworkMessages::GameEnded>(m_pBitStream);

      HandleGameEnded();
    }
    break;

  case S_REQUEST_CHARINFO:
    {
      NetworkMessages::RequestCharacterInfo *msgRequestCharacterInfo = ParseMessage<NetworkMessages::RequestCharacterInfo>(m_pBitStream);

      HandleRequestCharacterInfo();
    }
    break;

  case S_REPLY_CHARID:
    {
      NetworkMessages::ReplyCharacterId *msgReplyCharacterId = ParseMessage<NetworkMessages::ReplyCharacterId>(m_pBitStream);

      HandleReplyCharacterId(msgReplyCharacterId);
    }
    break;

  case S_REPLY_EQUIPMENT_ID:
    {
      NetworkMessages::EquipmentSetID *msgEquipmentSetID = ParseMessage<NetworkMessages::EquipmentSetID>(m_pBitStream);

      HandleReplyEquipmentID(msgEquipmentSetID);
    }
    break;

  case S_PUSH_NEWCHAR:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::Character *msgCharacter = ParseMessage<NetworkMessages::Character>(m_pBitStream);

      HandleCharacterCreation(msgCharacter);
    }
    break;

  case S_PUSH_NEWEQUIPMENT:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::Equipment *msgEquipment = ParseMessage<NetworkMessages::Equipment>(m_pBitStream);
      
      HandleEquipmentCreation(msgEquipment); 
    }
    break;

  case S_PUSH_ADDCHARMINION:
    break;

  case S_PUSH_SETCHARALIGNMENT:
    break;

  case S_PUSH_EQUIPMENT_DROP:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::EquipmentDrop *msgEquipmentDrop = ParseMessage<NetworkMessages::EquipmentDrop>(m_pBitStream);
      NetworkMessages::Position msgPosition = msgEquipmentDrop->position().Get(0);

      Vector3 position;
      position.x = msgPosition.x();
      position.y = msgPosition.y();
      position.z = msgPosition.z();

      u32 equipmentId = msgEquipmentDrop->equipmentid();
      bool unk0 = msgEquipmentDrop->unk0();

      HandleEquipmentDrop(equipmentId, position, unk0);
    }
    break;

  case S_PUSH_EQUIPMENT_PICKUP:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::EquipmentPickup *msgEquipmentPickup = ParseMessage<NetworkMessages::EquipmentPickup>(m_pBitStream);

      u32 equipmentId = msgEquipmentPickup->equipmentid();
      u32 characterId = msgEquipmentPickup->characterid();

      HandleEquipmentPickup(characterId, equipmentId);
    }
    break;

  case S_PUSH_EQUIPMENT_EQUIP:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::InventoryAddEquipment *msgInventoryAddEquipment = ParseMessage<NetworkMessages::InventoryAddEquipment>(m_pBitStream);
      u32 ownerId = msgInventoryAddEquipment->ownerid();
      u32 equipmentId = msgInventoryAddEquipment->equipmentid();
      u32 slot = msgInventoryAddEquipment->slot();
      u32 unk0 = msgInventoryAddEquipment->unk0();

      HandleInventoryAddEquipment(ownerId, equipmentId, slot, unk0);
    }
    break;

  case S_PUSH_EQUIPMENT_UNEQUIP:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::InventoryRemoveEquipment *msgInventoryRemoveEquipment = ParseMessage<NetworkMessages::InventoryRemoveEquipment>(m_pBitStream);
      u32 ownerId = msgInventoryRemoveEquipment->ownerid();
      u32 equipmentId = msgInventoryRemoveEquipment->equipmentid();

      HandleInventoryRemoveEquipment(ownerId, equipmentId);
    }
    break;

  case S_PUSH_EQUIPMENT_USE:
    break;

  case S_PUSH_EQUIPMENT_IDENTIFY:
    break;

  case S_PUSH_EQUIPMENT_ADDENCHANT:
    break;

  case S_PUSH_EQUIPMENT_ADDSOCKET:
    break;

  case S_PUSH_CHARACTER_SETDEST:
    {
      // Ignore server character creation if we're not in the game
      if (!gameClient->inGame) {
        break;
      }

      NetworkMessages::CharacterDestination *msgCharacterDestination = ParseMessage<NetworkMessages::CharacterDestination>(m_pBitStream);
      NetworkMessages::Position msgDestination = msgCharacterDestination->destination().Get(0);

      u32 id = msgCharacterDestination->id();

      Vector3 destination;
      destination.x = msgDestination.x();
      destination.z = msgDestination.z();

      u8 running = msgCharacterDestination->running();
      u8 attacking = msgCharacterDestination->attacking();

      HandleCharacterDestination(id, destination, running, attacking);
    }
    break;

  case S_PUSH_CHARACTER_ACTION:
    {
      NetworkMessages::CharacterAction *msgCharacterAction = ParseMessage<NetworkMessages::CharacterAction>(m_pBitStream);

      HandleCharacterSetAction(msgCharacterAction);
    }
    break;

    
  case S_PUSH_CHARACTER_ATTACK:
    {
      /* Hold off on this for now
      NetworkMessages::CharacterAttack *msgCharacterAttack = ParseMessage<NetworkMessages::CharacterAttack>(m_pBitStream);

      HandleCharacterAttack(msgCharacterAttack);
      */
    }
    break;

  case S_PUSH_CHARACTER_USESKILL:
    {
      NetworkMessages::CharacterUseSkill *msgCharacterUseSkill = ParseMessage<NetworkMessages::CharacterUseSkill>(m_pBitStream);

      HandleCharacterUseSkill(msgCharacterUseSkill);
    }
    break;

  case S_PUSH_EQUIPMENT_STACKUPDATE:
    {
      NetworkMessages::EquipmentUpdateStackSize *msgEquipmentUpdateStackSize = ParseMessage<NetworkMessages::EquipmentUpdateStackSize>(m_pBitStream);

      HandleEquipmentUpdateStack(msgEquipmentUpdateStackSize);
    }
    break;

  case S_PUSH_EQUIPMENT_ADD_GEM:
    {
      NetworkMessages::EquipmentAddGem *msgEquipmentAddGem = ParseMessage<NetworkMessages::EquipmentAddGem>(m_pBitStream);

      HandleEquipmentAddGem(msgEquipmentAddGem);
    }
    break;

  case S_PUSH_EQUIPMENT_REMOVE_GEMS:
    {
      NetworkMessages::EquipmentRemoveGems *msgEquipmentRemoveGems = ParseMessage<NetworkMessages::EquipmentRemoveGems>(m_pBitStream);

      HandleEquipmentRemoveGems(msgEquipmentRemoveGems);
    }
    break;
    
  case S_PUSH_CHAT_PLAYER:
    {
      NetworkMessages::ChatPlayerText *msgChatPlayerText = ParseMessage<NetworkMessages::ChatPlayerText>(m_pBitStream);

      HandleChatMessage(msgChatPlayerText);
    }
    break;

  }     
}

//
// Below are the message handlers

// Receives a version from the server, replies with it's own version
void Client::HandleVersion(u32 version)
{
  multiplayerLogger.WriteLine(Info, L"Client received Server Version: %i", version);

  NetworkMessages::Version msgVersion;
  msgVersion.set_version(MessageVersion);

  SendMessage<NetworkMessages::Version>(C_VERSION, &msgVersion);

  // After we send, check if they're the same
  if (version != MessageVersion) {
    multiplayerLogger.WriteLine(Info, L"Disconnecting from the Server: Version Mismatch: (Client = %i  Server = %i)",
      MessageVersion, version);

    Disconnect();
  } else {
    // If they're Ok, Request whether the Game has started yet
    NetworkMessages::GameHasStarted msgGameHasStarted;

    SendMessage<NetworkMessages::GameHasStarted>(C_REQUEST_HASGAMESTARTED, &msgGameHasStarted);
  }
}

// Receives from server whether the game has started
// If it has, it's safe to enter the game, else block player from entering.
void Client::HandleHasGameStarted(bool gameStarted)
{
  Client::getSingleton().SetServerGameStarted(gameStarted);
}

// Receives a push from the server when it finally starts the game.
void Client::HandleGameStarted()
{
  Client::getSingleton().SetServerGameStarted(true);

  // TODO Show Window Dialog informing user the server has started and they can enter.
}

// Receives a push from server when the game has been stopped (Exited to MainMenu)
void Client::HandleGameEnded()
{
  Client::getSingleton().SetServerGameStarted(false);
  
  // TODO Exit Player to MainMenu
}

// Receives a request from the Server for Character Info
void Client::HandleRequestCharacterInfo()
{
  NetworkMessages::ReplyCharacterInfo msgReplyCharacterInfo;

  NetworkMessages::Character *msgPlayer         = msgReplyCharacterInfo.mutable_player();
  NetworkMessages::Position  *msgPlayerPosition = msgPlayer->mutable_position();

  // Add the player information
  CPlayer *player = gameClient->pCPlayer;
  string playerName(player->characterName.begin(), player->characterName.end());
  playerName.assign(player->characterName.begin(), player->characterName.end());

  msgPlayer->set_guid(player->GUID);
  msgPlayer->set_name(playerName);
  msgPlayer->set_level(player->level);

  // This allows a workaround for the modified attributes needed for equipment
  /*
  msgPlayer->set_strength(player->baseStrength);
  msgPlayer->set_dexterity(player->baseDexterity);
  msgPlayer->set_defense(player->baseDefense);
  msgPlayer->set_magic(player->baseMagic);
  */
  msgPlayer->set_strength(10000);
  msgPlayer->set_dexterity(10000);
  msgPlayer->set_defense(10000);
  msgPlayer->set_magic(10000);
  msgPlayer->set_health(player->healthMax);
  msgPlayer->set_mana(player->manaMax);

  msgPlayerPosition->set_x(player->position.x);
  msgPlayerPosition->set_y(player->position.y);
  msgPlayerPosition->set_z(player->position.z);

  // Add the Character's Skills from both lists
  CSkillManager *skillManager = player->pCSkillManager;  
  for (u32 i = 0; i < skillManager->listSkills0.size; i++) {
    CSkill *skill = skillManager->listSkills0[i];
    string *skillName = msgPlayer->add_skills();

    skillName->assign(skill->pCSkillProperty0->skillName.begin(), skill->pCSkillProperty0->skillName.end());
  }
  for (u32 i = 0; i < skillManager->listSkills1.size; i++) {
    CSkill *skill = skillManager->listSkills1[i];
    string *skillName = msgPlayer->add_skills();

    skillName->assign(skill->pCSkillProperty0->skillName.begin(), skill->pCSkillProperty0->skillName.end());
  }
  
  // Add Minions
  vector<CCharacter*> *minionList = gameClient->pCPlayer->GetMinions();
  vector<CCharacter*>::iterator itr;
  for (itr = minionList->begin(); itr != minionList->end(); itr++) {
    string petName((*itr)->characterName.begin(), (*itr)->characterName.end());
    petName.assign((*itr)->characterName.begin(), (*itr)->characterName.end());

    NetworkMessages::Character *msgPet = msgPlayer->add_minion();
    msgPet->set_guid((*itr)->GUID);
    msgPet->set_name(petName);
    msgPet->set_level((*itr)->level);
  }

  Client::getSingleton().SendMessage<NetworkMessages::ReplyCharacterInfo>(C_REPLY_CHARINFO, &msgReplyCharacterInfo);

  Helper_ClientPushAllEquipment();
}

// Receives a Character Network ID from the server to apply to our character
// This is really a special message since the Client creates the player without the server
// telling it to
void Client::HandleReplyCharacterId(TLMP::NetworkMessages::ReplyCharacterId *msgReplyCharacterId)
{
  CPlayer *player = gameClient->pCPlayer;

  // Update our player or minion with the Network ID
  if (player->GUID == msgReplyCharacterId->guid()) {
    NetworkEntity *newEntity = new NetworkEntity(player, msgReplyCharacterId->id());
    NetworkSharedCharacters->push_back(newEntity);
  } else {
    vector<CCharacter*> *minions = player->GetMinions();
    vector<CCharacter*>::iterator itr;
    for (itr = minions->begin(); itr != minions->end(); itr++) {
      if ((*itr)->GUID == msgReplyCharacterId->guid()) {
        NetworkEntity *newEntity = new NetworkEntity((*itr), msgReplyCharacterId->id());
        NetworkSharedCharacters->push_back(newEntity);

        break;
      }
    }
  }  
}

// Pushes player inventory equipment list to the server to create
void Client::PushEquipment()
{
}

// Handles Character Set Destination
void Client::HandleCharacterDestination(u32 commonId, Vector3 destination, u8 running, u8 attacking)
{
  //multiplayerLogger.WriteLine(Info, L"Client received character setDestination (CommonID = %x), Position = %f, %f",
  //  commonId, destination.x, destination.z);
  //log(L"Client received character setDestination (CommonID = %x), Position = %f, %f",
  //  commonId, destination.x, destination.z);

  // Search for our Character given the CommonID
  NetworkEntity *entity = searchCharacterByCommonID(commonId);

  if (!entity) {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find internal object in shared characters from commonId: %x when receiving character destination.", commonId);
    log(L"Error: Could not find internal object in shared characters from commonId: %x when receiving character destination.", commonId);
    return;
  }

  CCharacter *character = (CCharacter *)entity->getInternalObject();

  if (character) {
    // Lock the Client from sending a network message back out when setting the Character Destination
    Client::getSingleton().SetSuppressed_SetDestination(true);
    character->SetDestination(gameClient->pCLevel, destination.x, destination.z);
    Client::getSingleton().SetSuppressed_SetDestination(false);

    character->running = running;
    character->attacking = attacking;
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Character is NULL");
    log(L"Error: Character is NULL");
  }
}

// Handles Character Creation
void Client::HandleCharacterCreation(NetworkMessages::Character *msgCharacter)
{
  NetworkMessages::Position msgCharacterPosition = msgCharacter->position();

  Vector3 posCharacter;
  posCharacter.x = msgCharacterPosition.x();
  posCharacter.y = msgCharacterPosition.y();
  posCharacter.z = msgCharacterPosition.z();

  u32 commonId = msgCharacter->id();
  u32 health = msgCharacter->health();
  u32 mana = msgCharacter->mana();
  u32 levelCharacter = msgCharacter->level();
  u32 strength = msgCharacter->strength();
  u32 dexterity = msgCharacter->dexterity();
  u32 defense = msgCharacter->defense();
  u32 magic = msgCharacter->magic();
  u64 guidCharacter = msgCharacter->guid();
  string characterName = msgCharacter->name();


  multiplayerLogger.WriteLine(Info, L"Client received character creation: (CommonID = %x) (GUID = %016I64X, name = %s)",
    commonId, guidCharacter, TLMP::convertAsciiToWide(characterName).c_str());

  CResourceManager *resourceManager = (CResourceManager *)gameClient->pCPlayer->pCResourceManager;
  CLevel *level = gameClient->pCLevel;

  // Create the monster, setup name and alignment
  Client::getSingleton().SetSuppressed_CharacterCreation(false);
  CMonster* monster = resourceManager->CreateMonster(guidCharacter, levelCharacter, true);
  
  if (monster) {
    monster->characterName.assign(convertAsciiToWide(characterName));
    monster->SetAlignment(1);
    level->CharacterInitialize(monster, &posCharacter, 0);
    monster->healthMax = health;
    monster->manaMax = mana;
    monster->baseStrength = strength;
    monster->baseDexterity = dexterity;
    monster->baseDefense = defense;
    monster->baseMagic = magic;
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Character created was null!");
  }

  Client::getSingleton().SetSuppressed_CharacterCreation(true);

  // Create a network ID to identify this monster later
  NetworkEntity *newEntity = addCharacter(monster, commonId);
}

void Client::HandleEquipmentCreation(TLMP::NetworkMessages::Equipment *msgEquipment)
{
  u32 id = msgEquipment->id();
  u64 guid = msgEquipment->guid();
  u32 stacksize = msgEquipment->stacksize();
  u32 stacksizemax = msgEquipment->stacksizemax();
  u32 socketcount = msgEquipment->socketcount();
  u32 gemCount = msgEquipment->gems_size();
  u32 enchantCount = msgEquipment->enchants_size();

  u32 minPhysicalDamage = msgEquipment->physical_damage_min();
  u32 maxPhysicalDamage = msgEquipment->physical_damage_max();

  wstring nameUnidentified = convertAsciiToWide(msgEquipment->name_unidentified());
  wstring namePrefix = convertAsciiToWide(msgEquipment->name_prefix());
  wstring nameSuffix = convertAsciiToWide(msgEquipment->name_suffix());

  u32 reqLevel = msgEquipment->req_level();
  u32 reqStrength = msgEquipment->req_strength();
  u32 reqDexterity = msgEquipment->req_dexterity();
  u32 reqMagic = msgEquipment->req_magic();
  u32 reqDefense = msgEquipment->req_defense();


  multiplayerLogger.WriteLine(Info, L"Client received equipment creation: (CommonID = %x) (GUID = %016I64X) (Stack: %i/%i) (SocketCount: %i)",
    id, guid, stacksize, stacksizemax, socketcount);
  log(L"Client received equipment creation: (CommonID = %x) (GUID = %016I64X) (Stack: %i/%i) (SocketCount: %i)",
    id, guid, stacksize, stacksizemax, socketcount);

  CResourceManager *resourceManager = (CResourceManager *)gameClient->pCPlayer->pCResourceManager;

  if (resourceManager) {
    CEquipment *equipment = resourceManager->CreateEquipment(guid, 1, 1, 0);
    equipment->socketCount = socketcount;
    equipment->stackSize = stacksize;
    equipment->stackSizeMax = stacksizemax;

    equipment->minimumPhysicalDamage = minPhysicalDamage;
    equipment->maximumPhysicalDamage = maxPhysicalDamage;
    equipment->changeMeForDamageUpdate = maxPhysicalDamage;

    equipment->nameUnidentified = nameUnidentified;
    equipment->namePrefix = namePrefix;
    equipment->nameSuffix = nameSuffix;
    equipment->requirements[0] = reqLevel;
    equipment->requirements[1] = reqStrength;
    equipment->requirements[2] = reqDexterity;
    equipment->requirements[3] = reqMagic;
    equipment->requirements[4] = reqDefense;
    
    multiplayerLogger.WriteLine(Info, L"Client: Adding gems to new equipment...");
    log(L"Client: Adding gems to new equipment...");

    // Add the gems
    for (u32 i = 0; i < gemCount; i++) {
      NetworkMessages::Equipment msgGem = msgEquipment->gems().Get(i);
      CEquipment *gem = resourceManager->CreateEquipment(msgGem.guid(), 1, 1, 0);
      
      multiplayerLogger.WriteLine(Info, L"Client: Adding gem to shared network equipment: %i", msgGem.id());
      log(L"Client: Adding gem to shared network equipment: %i", msgGem.id());
      NetworkEntity *newEntity = addEquipment(gem, msgGem.id());

      for (int j = 0; j < msgGem.enchants_size(); j++) {
        NetworkMessages::EnchantType msgGemEnchantType = msgGem.enchants().Get(j);
        gem->AddEnchant((EffectType)msgGemEnchantType.type(), (EnchantType)msgGemEnchantType.subtype(), msgGemEnchantType.value());
      }

      equipment->gemList.push(gem);
    }

    multiplayerLogger.WriteLine(Info, L"Client: Adding enchants to new equipment... count = %i", enchantCount);
    log(L"Client: Adding enchants to new equipment... count = %i", enchantCount);

    // Add the enchants
    for (u32 i = 0; i < enchantCount; i++) {
      NetworkMessages::EnchantType msgEnchantType = msgEquipment->enchants().Get(i);

      multiplayerLogger.WriteLine(Info, L"Client: Enchant %i: Type: %x, SubType: %x, Value: %i", i, msgEnchantType.type(), msgEnchantType.subtype(), msgEnchantType.value());
      log(L"Client: Enchant %i: Type: %x, SubType: %x, Value: %i", i, msgEnchantType.type(), msgEnchantType.subtype(), msgEnchantType.value());

      equipment->AddEnchant((EffectType)msgEnchantType.type(), (EnchantType)msgEnchantType.subtype(), msgEnchantType.value());
    }

    multiplayerLogger.WriteLine(Info, L"Client: Adding equipment to shared network equipment");
    log(L"Client: Adding equipment to shared network equipment");
    NetworkEntity *newEntity = addEquipment(equipment, id);
  }
}

void Client::HandleInventoryAddEquipment(u32 ownerId, u32 equipmentId, u32 slot, u32 unk0)
{
  multiplayerLogger.WriteLine(Info, L"Client received inventory add equipment: (CharacterID = %x, EquipmentID = %x, slot = %x)",
    ownerId, equipmentId, slot);
  log(L"Client received inventory add equipment: (CharacterID = %x, EquipmentID = %x, slot = %x)",
    ownerId, equipmentId, slot);

  NetworkEntity* owner = searchCharacterByCommonID(ownerId);
  NetworkEntity* equipment = searchEquipmentByCommonID(equipmentId);

  if (owner) {
    if (equipment) {
      CCharacter *characterOwner = (CCharacter*)owner->getInternalObject();
      CEquipment *equipmentReal = (CEquipment*)equipment->getInternalObject();
      CInventory *inventory = characterOwner->pCInventory;

      log(L"Debug: Owner: %p Equipment: %p Inventory: %p Slot: %x Unk0: %x",
        characterOwner, equipmentReal, inventory, slot, unk0);
      log(L"  Character: %s", characterOwner->characterName.c_str());
      log(L"  Equipment: %s", equipmentReal->nameReal.c_str());
      log(L"  Inventory EquipmentList Size: %x", inventory->equipmentList.size);

      Client::getSingleton().SetSuppressed_EquipmentCreation(false);
      inventory->AddEquipment(equipmentReal, slot, unk0);
      Client::getSingleton().SetSuppressed_EquipmentCreation(true);
    } else {
      multiplayerLogger.WriteLine(Error, L"Error: Could not find Equipment with ID = %x", equipmentId);
      log(L"Error: Could not find Equipment with ID = %x", equipmentId);
    }
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find Character with ID = %x", ownerId);
    log(L"Error: Could not find Character with ID = %x", ownerId);
  }

  log(L"HandleInventoryAddEquipment Done.");
}

void Client::HandleInventoryRemoveEquipment(u32 ownerId, u32 equipmentId)
{
  multiplayerLogger.WriteLine(Info, L"Client received inventory remove equipment: (CharacterID = %x, EquipmentID = %x)",
    ownerId, equipmentId);
  log(L"Client received inventory remove equipment: (CharacterID = %x, EquipmentID = %x)",
    ownerId, equipmentId);

  NetworkEntity* owner = searchCharacterByCommonID(ownerId);
  NetworkEntity* equipment = searchEquipmentByCommonID(equipmentId);

  if (owner) {
    if (equipment) {
      CCharacter *characterOwner = (CCharacter*)owner->getInternalObject();
      CEquipment *equipmentReal = (CEquipment*)equipment->getInternalObject();

      CInventory *inventory = characterOwner->pCInventory;

      if (inventory) {
        // Suppress from sending this out again
        SetSuppressed_SendEquipmentUnequip(true);
        inventory->RemoveEquipment(equipmentReal);
        SetSuppressed_SendEquipmentUnequip(false);
      } else {
        multiplayerLogger.WriteLine(Error, L"Error: Inventory was NULL.");
        log(L"Error: Inventory was NULL.");
      }
    } else {
      multiplayerLogger.WriteLine(Error, L"Error: Could not find Equipment with ID = %x", equipmentId);
      log(L"Error: Could not find Equipment with ID = %x", equipmentId);
    }
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find Character with ID = %x", ownerId);
    log(L"Error: Could not find Character with ID = %x", ownerId);
  }
}

void Client::HandleEquipmentDrop(u32 equipmentId, Vector3 position, bool unk0)
{
  multiplayerLogger.WriteLine(Info, L"Client received Equipment Drop: (EquipmentId = %x, Position: %f, %f, %f, Unk0: %i",
    equipmentId, position.x, position.y, position.z, unk0);
  log(L"Client received Equipment Drop: (EquipmentId = %x, Position: %f, %f, %f, Unk0: %i",
    equipmentId, position.x, position.y, position.z, unk0);

  NetworkEntity *netEquipment = searchEquipmentByCommonID(equipmentId);

  if (netEquipment) {
    CEquipment *equipment = (CEquipment *)netEquipment->getInternalObject();
    CLevel *level = gameClient->pCLevel;

    Client::getSingleton().SetSuppressed_EquipmentDrop(true);
    level->EquipmentDrop(equipment, position, unk0);
    Client::getSingleton().SetSuppressed_EquipmentDrop(false);
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find Equipment from CommonId: %x",
      equipmentId);
    log(L"Error: Could not find Equipment from CommonId: %x",
      equipmentId);
  }
}

void Client::HandleEquipmentPickup(u32 characterId, u32 equipmentId)
{
  NetworkEntity *netEquipment = searchEquipmentByCommonID(equipmentId);
  NetworkEntity *netCharacter = searchCharacterByCommonID(characterId);

  if (netEquipment) {
    if (netCharacter) {
      CEquipment *equipment = (CEquipment *)netEquipment->getInternalObject();
      CCharacter *character = (CCharacter *)netCharacter->getInternalObject();

      SetSuppressed_EquipmentPickup(false);
      character->PickupEquipment(equipment, gameClient->pCLevel);
      SetSuppressed_EquipmentPickup(true);
    } else {
      multiplayerLogger.WriteLine(Error, L"Error: Could not find Character from CommonId: %x", characterId);
      log(L"Error: Could not find Character from CommonId: %x", characterId);
    }
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find Equipment from CommonId: %x", equipmentId);
    log(L"Error: Could not find Equipment from CommonId: %x", equipmentId);
  }
}

void Client::HandleReplyEquipmentID(NetworkMessages::EquipmentSetID *msgEquipmentSetID)
{
  u64 guid = msgEquipmentSetID->guid();
  u32 client_id = msgEquipmentSetID->client_id();
  u32 id = msgEquipmentSetID->id();

  multiplayerLogger.WriteLine(Info, L"  Client: Received a commonID from server: %016I64X in Slot: %x  ID = %x",
    guid, client_id, id);
  log(L"  Client: Received a commonID from server: %016I64X in Slot: %x  ID = %x",
    guid, client_id, id);

  // Search for the matching Client ID
  NetworkEntity *clientEquipment = searchEquipmentByClientID(client_id);

  if (clientEquipment) {
    // Remove the Client temp equipment and create the actual network ID with the
    // appropriate common ID
    CEquipment *equipment = (CEquipment*)clientEquipment->getInternalObject();
    removeEquipmentByClientID(client_id);

    addEquipment(equipment, id);

    // If the equipment is equipped on the body - tell the server to add it
    NetworkEntity *netCharacter = searchCharacterByInternalObject(gameClient->pCPlayer);
    if (netCharacter) {
      if (gameClient->pCPlayer->pCInventory) {
        u32 slotBody = gameClient->pCPlayer->pCInventory->GetEquipmentSlot(equipment);
        if (slotBody <= 0x0C) {
          NetworkMessages::InventoryAddEquipment msgInventoryAddEquipment;
          msgInventoryAddEquipment.set_equipmentid(id);
          msgInventoryAddEquipment.set_guid(equipment->GUID);
          msgInventoryAddEquipment.set_ownerid(netCharacter->getCommonId());
          msgInventoryAddEquipment.set_slot(slotBody);
          msgInventoryAddEquipment.set_unk0(1);

          Client::getSingleton().SendMessage<NetworkMessages::InventoryAddEquipment>(C_PUSH_EQUIPMENT_EQUIP, &msgInventoryAddEquipment);
        }
      } else {
        log(L"Error: Character has no Inventory!");
        multiplayerLogger.WriteLine(Error, L"Error: Character has no Inventory!");
      }
    }

  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Expected to set common ID for client Equipment, but could not find the Equipment with Client ID = %x",
      client_id);
    log(L"Error: Expected to set common ID for client Equipment, but could not find the Equipment with Client ID = %x",
      client_id);
  }

  // Set the common ID for the Equipment
}

void Client::Helper_ClientPushAllEquipment()
{
  // Search Player and Minion inventories for unknown Common IDs for the Network
  // Any not found will be pushed to the server so we can get them

  CPlayer *player = gameClient->pCPlayer;
  vector<CCharacter*>* clientCharacters = Helper_ProduceClientSideCharacters(player);

  multiplayerLogger.WriteLine(Info, L"Pushing all equipment to server, Client Character Count: %i",
    clientCharacters->size());
  logColor(B_RED, L"Pushing all equipment to server, Client Character Count: %i",
    clientCharacters->size());

  vector<CCharacter*>::iterator itr;
  for (itr = clientCharacters->begin(); itr != clientCharacters->end(); itr++) {
    CInventory *characterInventory = (*itr)->pCInventory;

    multiplayerLogger.WriteLine(Info, L"  Moving through inventory of size: %i",
      characterInventory->equipmentList.size);
    log(L"  Moving through inventory of size: %i",
      characterInventory->equipmentList.size);

    for (u32 i = 0; i < characterInventory->equipmentList.size; i++) {
      CEquipment* equipment = characterInventory->equipmentList[i]->pCEquipment;

      // Check if the Equipment has a valid common ID, if not add to temp vec and push to server
      NetworkEntity *existingEquipment = searchEquipmentByInternalObject(equipment);

      if (!existingEquipment) {
        multiplayerLogger.WriteLine(Info, L"    Equipment doesn't have a common ID, creating temp and pushing to server.");
        log(L"    Equipment doesn't have a common ID, creating temp and pushing to server.");

        Helper_ClientPushEquipment(equipment);
      } else {
        multiplayerLogger.WriteLine(Info, L"    Equipment already has a Common ID of: %x",
          existingEquipment->getCommonId());
        log(L"    Equipment already has a Common ID of: %x",
          existingEquipment->getCommonId());
      }
    }
  }
}

void Client::Helper_ClientPushEquipment(CEquipment *equipment)
{
  // Add this equipment to our temporary list
  NetworkEntity *clientEquipment = addEquipmentClientTemporary(equipment);

  multiplayerLogger.WriteLine(Info, L"Pushing Equipment to Server (Client Temp ID = %x)",
    clientEquipment->getCommonId());
  log(L"Pushing Equipment to Server (Client Temp ID = %x)",
    clientEquipment->getCommonId());

  NetworkMessages::Equipment msgEquipment;
  Helper_PopulateEquipmentMessage(&msgEquipment, equipment, clientEquipment);

  log(L"  DEBUG: Equipment gemList: %p (Offset = %x)",
    &equipment->gemList, ((u32)&equipment->gemList - (u32)equipment));
  multiplayerLogger.WriteLine(Info, L"  Equipment contains %i Gems", equipment->gemList.size);
  log(L"  Equipment contains %i Gems", equipment->gemList.size);


  // Add the Gem Equipment
  for (u32 i = 0; i < equipment->gemList.size; i++) {
    multiplayerLogger.WriteLine(Info, L"Client: Adding gem...");
    log(L"Client: Adding gem...");

    NetworkMessages::Equipment *msgGem = msgEquipment.add_gems();
    CEquipment *gem = equipment->gemList[i];

    NetworkEntity *netGem = addEquipmentClientTemporary(gem);
    Helper_PopulateEquipmentMessage(msgGem, gem, netGem);
  }

  Client::getSingleton().SendMessage<NetworkMessages::Equipment>(C_PUSH_EQUIPMENT, &msgEquipment);
}

vector<CCharacter*>* Client::Helper_ProduceClientSideCharacters(CCharacter *character)
{
  vector<CCharacter*>* retval = new vector<CCharacter*>();
  vector<CCharacter*>* minions = character->GetMinions();
  vector<CCharacter*>* minionList;

  retval->push_back(character);

  vector<CCharacter*>::iterator itr, itrMinion;
  for (itr = minions->begin(); itr != minions->end(); itr++) {
    minionList = Helper_ProduceClientSideCharacters((*itr));
    for (itrMinion = minionList->begin(); itrMinion != minionList->end(); itrMinion++) {
      retval->push_back((*itrMinion));
    }
    delete minionList;
  }

  return retval;
}

void Client::HandleCharacterSetAction(NetworkMessages::CharacterAction* msgCharacterAction)
{
  u32 id = msgCharacterAction->characterid();
  u32 action = msgCharacterAction->action();

  NetworkEntity *networkCharacter = searchCharacterByCommonID(id);

  if (networkCharacter) {
    CCharacter* character = (CCharacter*)networkCharacter->getInternalObject();

    // Stop the suppress and network request
    Client::getSingleton().SetSuppressed_CharacterAction(false);
    character->SetAction(action);
    Client::getSingleton().SetSuppressed_CharacterAction(true);
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find character with common id = %x", id);
    log(L"Error: Could not find character with common id = %x", id);
  }
}

void Client::HandleCharacterAttack(NetworkMessages::CharacterAttack* msgCharacterAttack)
{
  u32 id = msgCharacterAttack->characterid();

  NetworkEntity *networkCharacter = searchCharacterByCommonID(id);

  if (networkCharacter) {
    CCharacter* character = (CCharacter*)networkCharacter->getInternalObject();

    // Stop the suppress and network request
    Client::getSingleton().SetSuppressed_CharacterAttack(false);
    character->Attack();
    Client::getSingleton().SetSuppressed_CharacterAttack(true);
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find character with common id = %x", id);
    log(L"Error: Could not find character with common id = %x", id);
  }
}

void Client::HandleCharacterUseSkill(NetworkMessages::CharacterUseSkill* msgCharacterUseSkill)
{
  u32 id = msgCharacterUseSkill->characterid();
  u64 skill = msgCharacterUseSkill->skill();
  Vector3 direction;

  NetworkMessages::Position msgPosition = msgCharacterUseSkill->direction();
  direction.x = msgPosition.x();
  direction.y = msgPosition.y();
  direction.z = msgPosition.z();

  log(L"Client HandleCharacterUseSkill: %f %f %f", 
    direction.x, direction.y, direction.z);

  NetworkEntity *networkCharacter = searchCharacterByCommonID(id);
  if (networkCharacter) {
    CCharacter* character = (CCharacter*)networkCharacter->getInternalObject();

    // Stop the suppress and network request
    Client::getSingleton().SetSuppressed_SendUseSkill(true);
    character->UseSkill(skill);
    Client::getSingleton().SetSuppressed_SendUseSkill(false);
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find character with common id = %x", id);
    log(L"Error: Could not find character with common id = %x", id);
  }
}

void Client::HandleEquipmentUpdateStack(NetworkMessages::EquipmentUpdateStackSize *msgEquipmentUpdateStackSize)
{
  u32 id = msgEquipmentUpdateStackSize->id();
  u32 amount = msgEquipmentUpdateStackSize->change_amount();

  NetworkEntity *netEquipment = searchEquipmentByCommonID(id);

  if (netEquipment) {
    CEquipment *equipment = (CEquipment*)netEquipment->getInternalObject();

    // Suppress the client from re-sending it
    SetSuppressed_SendEquipmentStack(true);
    EquipmentAddStackCount(equipment, amount);
    SetSuppressed_SendEquipmentStack(false);
  } else {
    multiplayerLogger.WriteLine(Error, L"Error: Could not find equipment with common id = %x", id);
    log(L"Error: Could not find equipment with common id = %x", id);
  }
}

void Client::HandleEquipmentAddGem(NetworkMessages::EquipmentAddGem* msgEquipmentAddGem)
{
  u32 equipmentId = msgEquipmentAddGem->equipmentid();
  u32 gemId = msgEquipmentAddGem->gemid();

  NetworkEntity *netEquipment = searchEquipmentByCommonID(equipmentId);
  NetworkEntity *netGem = searchEquipmentByCommonID(gemId);

  if (netEquipment && netGem) {
    CEquipment *equipment = (CEquipment*)netEquipment->getInternalObject();
    CEquipment *gem = (CEquipment*)netGem->getInternalObject();

    Set_IsSendingEquipmentAddGem(true);
    equipment->AddGem(gem);
    Set_IsSendingEquipmentAddGem(false);
  } else {
    log(L"Equipment or Gem commonId not found: %x %x  ptrs: %p %p",
      equipmentId, gemId, netEquipment, netGem);
    multiplayerLogger.WriteLine(Error, L"Equipment or Gem commonId not found: %x %x  ptrs: %p %p",
      equipmentId, gemId, netEquipment, netGem);
  }
}

void Client::HandleChatMessage(NetworkMessages::ChatPlayerText *msgChatPlayerText)
{
  log(L"Client: Received player chat text.");

  CEGUI::MultiLineEditbox *pWindow = (CEGUI::MultiLineEditbox *)getChatHistoryWindow();
  size_t selectionEnd;

  if (pWindow) {
    CEGUI::String message = msgChatPlayerText->text().c_str();
    u32 characterId = msgChatPlayerText->characterid();

    NetworkEntity *netCharacter = searchCharacterByCommonID(characterId);

    if (netCharacter) {
      CCharacter *character = (CCharacter*)netCharacter->getInternalObject();
      string characterName(character->characterName.begin(), character->characterName.end());
      characterName.assign(character->characterName.begin(), character->characterName.end());

      //selectionStart = pWindow->getText().length();
      pWindow->appendText(CEGUI::String(characterName) + CEGUI::String(": ") + message + CEGUI::String("\n"));
      selectionEnd = pWindow->getText().length();
      pWindow->setCaratIndex(selectionEnd);
    } else {
      multiplayerLogger.WriteLine(Error, L"Error could not find Network entity of id: %x", characterId);
    }
  }
}

//
// Populates the msgEquipment with the given Equipment vars
void Client::Helper_PopulateEquipmentMessage(NetworkMessages::Equipment* msgEquipment, CEquipment *equipment, NetworkEntity *netEquipment)
{
  multiplayerLogger.WriteLine(Info, L"  Setting up regular variables for Equipment: %s", equipment->nameReal.c_str());
  log(L"  Setting up regular variables for Equipment: %s", equipment->nameReal.c_str());

  // Fill out the easy stuff
  msgEquipment->set_guid(equipment->GUID);
  msgEquipment->set_id(netEquipment->getCommonId());
  msgEquipment->set_stacksize(equipment->stackSize);
  msgEquipment->set_stacksizemax(equipment->stackSizeMax);
  msgEquipment->set_socketcount(equipment->socketCount);
  msgEquipment->set_client_id(netEquipment->getCommonId());     // Create a temporary client ID to identify this when we receive the common ID

  msgEquipment->set_physical_damage_min(equipment->minimumPhysicalDamage);
  msgEquipment->set_physical_damage_max(equipment->maximumPhysicalDamage);

  // Check if we're a:
  //  Waypoint Portal
  //  Return To Dungeon
  // If so skip this
  if (equipment->GUID != 0x761772BDA01D11DE &&
    equipment->GUID != 0xD3A8F99E2FA111DE) 
  {
    string nameUnidentified(equipment->nameUnidentified.begin(), equipment->nameUnidentified.end());
    nameUnidentified.assign(equipment->nameUnidentified.begin(), equipment->nameUnidentified.end());
    msgEquipment->set_name_unidentified(nameUnidentified);

    string namePrefix(equipment->namePrefix.begin(), equipment->namePrefix.end());
    namePrefix.assign(equipment->namePrefix.begin(), equipment->namePrefix.end());
    msgEquipment->set_name_prefix(namePrefix);
    
    string nameSuffix(equipment->nameSuffix.begin(), equipment->nameSuffix.end());
    nameSuffix.assign(equipment->nameSuffix.begin(), equipment->nameSuffix.end());
    msgEquipment->set_name_suffix(nameSuffix);

    msgEquipment->set_req_level(equipment->requirements[0]);
    msgEquipment->set_req_strength(equipment->requirements[1]);
    msgEquipment->set_req_dexterity(equipment->requirements[2]);
    msgEquipment->set_req_magic(equipment->requirements[3]);
    msgEquipment->set_req_defense(equipment->requirements[4]);

    multiplayerLogger.WriteLine(Info, L"  Setting up Regular Enchants... size = %x",
      equipment->enchantList.size());
    log(L"  Setting up Regular Enchants... size = %x",
      equipment->enchantList.size());

    // New
    // Work on Regular Enchants
    for (size_t index = 0; index < equipment->enchantList.size(); index++) {
      log(L"  Adding Equipment Enchant: REGULAR");
      log(L"    (%i: %i)", equipment->enchantTypeList[index], equipment->enchantList[index]);
      multiplayerLogger.WriteLine(Info, L"  Adding Equipment Enchant: REGULAR");
      multiplayerLogger.WriteLine(Info, L"    (%i: %i)", equipment->enchantTypeList[index], equipment->enchantList[index]);

      NetworkMessages::EnchantType *msgEnchantType = msgEquipment->add_enchants();
      msgEnchantType->set_type(REGULAR);
      msgEnchantType->set_subtype(equipment->enchantTypeList[index]);
      msgEnchantType->set_value((float)equipment->enchantList[index]);
    }


    // Add the Effect Enchants
    CEffectManager *effectManager = equipment->pCEffectManager;
    if (effectManager) {
      multiplayerLogger.WriteLine(Info, L"  Setting up Effect Enchants...");
      log(L"  Setting up Effect Enchants...");

      // Work on Effect Enchants
      for (u32 i = 0; i < effectManager->effectList.size; i++) {
        CEffect *effect = effectManager->effectList[i];

        multiplayerLogger.WriteLine(Info, L"  Adding Equipment Effect Enchant: (%x: %f)", effect->effectType, effect->effectValue);
        log(L"  Adding Equipment Effect Enchant: (%x: %f)", effect->effectType, effect->effectValue);

        NetworkMessages::EnchantType *msgEnchantType = msgEquipment->add_enchants();
        msgEnchantType->set_type(effect->effectType);
        msgEnchantType->set_subtype(-1);  // Not used in this instance
        msgEnchantType->set_value(effect->effectValue);
      }
    }
  }
}

void Client::HandleEquipmentRemoveGems(NetworkMessages::EquipmentRemoveGems *msgEquipmentRemoveGems)
{
  u32 equipmentId = msgEquipmentRemoveGems->equipmentid();

  NetworkEntity *netEntity = searchEquipmentByCommonID(equipmentId);

  if (netEntity) {
    CEquipment* equipment = (CEquipment*)netEntity->getInternalObject();

    log(L"Client: Removing gems from equipment: %s", equipment->nameReal.c_str());
    multiplayerLogger.WriteLine(Info, L"Client: Removing gems from equipment: %s", equipment->nameReal.c_str());

    equipment->gemList.size = 0;
  } else {
    log(L"Error: Could not find NetworkEntity for equipment of id: %x", equipmentId);
    multiplayerLogger.WriteLine(Error, L"Error: Could not find NetworkEntity for equipment of id: %x", equipmentId);
  }
}