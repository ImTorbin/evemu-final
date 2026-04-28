-- +migrate Up
-- CC BY-SA: derived from https://wiki.eveuniversity.org/Combat_site
-- ids 9200000-9201450, rooms 19200000-19201450, count 146

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200000, 'Ancient Ruins', 0, 500022, 8, '093834bf-14da-40ee-aeec-fbf72dc21a28');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200000, 'Main', 9200000);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200000, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200010, 'Angel Burrow', 0, 500011, 7, '8fcccffb-946c-453b-97a5-12f45a06899e');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200010, 'Main', 9200010);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200010, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200020, 'Angel Den', 0, 500011, 7, '3cddb549-4936-45ab-aa21-ff8f3c6144cb');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200020, 'Main', 9200020);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200020, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200030, 'Angel Forlorn Den', 0, 500011, 7, '50f5f698-dfa1-4857-92b9-a0c72e636756');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200030, 'Main', 9200030);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200030, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200040, 'Angel Forlorn Hideaway', 0, 500011, 7, 'f7c6b597-6f1c-400c-a906-cd6ca0c4ea04');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200040, 'Main', 9200040);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200040, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200050, 'Angel Forlorn Hub', 0, 500011, 7, 'bf608cc6-95d4-4bad-8ce1-d55171dc4c15');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200050, 'Main', 9200050);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200050, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200060, 'Angel Forlorn Rally Point', 0, 500011, 7, 'efb1fa37-95c6-4195-9fe7-3daf2d15a10d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200060, 'Main', 9200060);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200060, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200070, 'Angel Forsaken Den', 0, 500011, 7, '774448f1-d0fd-488b-b885-7769367058ac');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200070, 'Main', 9200070);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200070, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200080, 'Angel Forsaken Hideaway', 0, 500011, 7, '70363255-1ca4-47a1-8c08-79326a5a63d9');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200080, 'Main', 9200080);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200080, 26127, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 26127 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200090, 'Angel Forsaken Hub', 0, 500011, 7, 'a1660940-f463-41fd-89fd-5e898e80c987');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200090, 'Main', 9200090);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200090, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200100, 'Angel Forsaken Rally Point', 0, 500011, 7, '00b73437-d598-4ab9-bba7-4971cfb9b24d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200100, 'Main', 9200100);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200100, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200110, 'Angel Forsaken Sanctum', 0, 500011, 7, '7b4f5627-38c8-424b-ae8f-702566beafc9');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200110, 'Main', 9200110);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200110, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200120, 'Angel Haven', 0, 500011, 7, '549994be-9fa3-4b6a-8052-ef16e0b60172');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200120, 'Main', 9200120);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200120, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200130, 'Angel Hidden Den', 0, 500011, 7, '7e8922bd-b131-45e6-9179-ee350aa5ef2a');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200130, 'Main', 9200130);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200130, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200140, 'Angel Hidden Hideaway', 0, 500011, 7, '3f8fdc2a-47b4-4666-b82f-2ab8b64829ea');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200140, 'Main', 9200140);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200140, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200150, 'Angel Hidden Hub', 0, 500011, 7, '28b4a747-2825-4dab-9546-5a6df2858365');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200150, 'Main', 9200150);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200150, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200160, 'Angel Hidden Rally Point', 0, 500011, 7, 'b9be3be1-3b0c-40d1-8da6-e2600a82974d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200160, 'Main', 9200160);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200160, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200170, 'Angel Hideaway', 0, 500011, 7, 'eead5c02-4ddb-4d71-a5d0-6b135b0d183c');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200170, 'Main', 9200170);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200170, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200180, 'Angel Hub', 0, 500011, 7, 'ea6cd920-9c93-484b-8a72-f02d34e093a0');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200180, 'Main', 9200180);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200180, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200190, 'Angel Port', 0, 500011, 7, '7fadd30e-d466-4ee5-a3dc-6cdfb4b522bc');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200190, 'Main', 9200190);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200190, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200200, 'Angel Rally Point', 0, 500011, 7, '7ee5625b-4bad-413b-b42b-152796533eee');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200200, 'Main', 9200200);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200200, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200210, 'Angel Refuge', 0, 500011, 7, 'b7b03c58-7ef6-40df-95bb-49b3a78422f2');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200210, 'Main', 9200210);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200210, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200220, 'Angel Sanctum', 0, 500011, 7, '1b324ea8-37f5-4a09-863a-e7d6709cda89');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200220, 'Main', 9200220);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200220, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200230, 'Angel Yard', 0, 500011, 7, 'bbaa1bf2-f1fc-44d6-9bcb-388b182a5453');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200230, 'Main', 9200230);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200230, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200240, 'Battleships', 0, 500022, 8, '3bca6c2c-8f94-4bcf-8005-0c621d0ae998');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200240, 'Main', 9200240);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 24692, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 24692 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 642, groupID, 15000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 642 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 643, groupID, 30000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 643 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 638, groupID, 45000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 638 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 24688, groupID, 60000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 24688 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 4306, groupID, 75000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 4306 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 640, groupID, 90000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 640 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 641, groupID, 105000.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 641 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 24690, groupID, 0.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 24690 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 645, groupID, 15000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 645 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 639, groupID, 30000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 639 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 644, groupID, 45000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 644 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 24694, groupID, 60000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 24694 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17726, groupID, 75000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17726 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 32305, groupID, 90000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 32305 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17636, groupID, 105000.0, 15000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17636 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 32309, groupID, 0.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 32309 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 32307, groupID, 15000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 32307 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17728, groupID, 30000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17728 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17732, groupID, 45000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17732 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 32311, groupID, 60000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 32311 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17920, groupID, 75000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17920 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17738, groupID, 90000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17738 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17736, groupID, 105000.0, 30000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17736 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17918, groupID, 0.0, 45000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17918 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 17740, groupID, 15000.0, 45000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 17740 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 28659, groupID, 30000.0, 45000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 28659 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 28710, groupID, 45000.0, 45000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 28710 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 28661, groupID, 60000.0, 45000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 28661 LIMIT 1;
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200240, 28665, groupID, 75000.0, 45000.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 28665 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200250, 'Bloated Ruins', 0, 500022, 8, '2f375a74-76a3-48b9-8f88-60949ff994e0');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200250, 'Main', 9200250);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200250, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200260, 'Blood Burrow', 0, 500012, 7, 'be8a73d4-c3ca-463a-86de-f38a00d61b89');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200260, 'Main', 9200260);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200260, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200270, 'Blood Den', 0, 500012, 7, '11b82f0a-4375-47ac-9a0f-e3988a1d7a7d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200270, 'Main', 9200270);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200270, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200280, 'Blood Forlorn Den', 0, 500012, 7, 'e323eade-f6dc-43ad-89ef-22e4e566a83f');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200280, 'Main', 9200280);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200280, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200290, 'Blood Forlorn Hideaway', 0, 500012, 7, 'db260ca6-9155-4b6c-a979-d79670ea55bb');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200290, 'Main', 9200290);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200290, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200300, 'Blood Forlorn Hub', 0, 500012, 7, '6d66664b-5862-4faa-81f3-00fc4f3e78f5');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200300, 'Main', 9200300);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200300, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200310, 'Blood Forlorn Rally Point', 0, 500012, 7, '640d1a85-39fa-4700-b30e-595e6c648a51');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200310, 'Main', 9200310);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200310, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200320, 'Blood Forsaken Den', 0, 500012, 7, '5ff758bd-9cff-4cb0-8f3b-2481bda8bae2');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200320, 'Main', 9200320);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200320, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200330, 'Blood Forsaken Hideaway', 0, 500012, 7, '8a2454f0-ba5e-4702-80bc-c8c9a60583e9');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200330, 'Main', 9200330);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200330, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200340, 'Blood Forsaken Hub', 0, 500012, 7, '9fad2450-3eac-4790-b308-3bcd7822789d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200340, 'Main', 9200340);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200340, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200350, 'Blood Forsaken Rally Point', 0, 500012, 7, '96bcb409-8f6d-4b1d-8e6c-9261a6847fea');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200350, 'Main', 9200350);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200350, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200360, 'Blood Forsaken Sanctum', 0, 500012, 7, '6c1af88c-5e00-42f7-9893-d895487c9242');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200360, 'Main', 9200360);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200360, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200370, 'Blood Haven', 0, 500012, 7, '45440cf4-d2d8-46d3-aad5-0a6edcb748c2');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200370, 'Main', 9200370);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200370, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200380, 'Blood Hidden Den', 0, 500012, 7, '7c684344-05b1-4e9c-adc1-7afc4ed3fc17');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200380, 'Main', 9200380);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200380, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200390, 'Blood Hidden Hideaway', 0, 500012, 7, '95eb1300-53d9-4a3f-b6c6-95f11048a3b6');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200390, 'Main', 9200390);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200390, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200400, 'Blood Hidden Hub', 0, 500012, 7, '2da87f1e-0e4d-4e33-898a-0e2da94eca42');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200400, 'Main', 9200400);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200400, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200410, 'Blood Hidden Rally Point', 0, 500012, 7, '7000e5de-1b5d-4094-89df-e601fd60c9ed');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200410, 'Main', 9200410);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200410, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200420, 'Blood Hideaway', 0, 500012, 7, '35aba95c-3760-4ee6-bc3e-0c8233e1de9a');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200420, 'Main', 9200420);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200420, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200430, 'Blood Hub', 0, 500012, 7, 'eac6b21d-89d4-4779-9ccd-5fac3f269ed8');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200430, 'Main', 9200430);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200430, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200440, 'Blood Port', 0, 500012, 7, 'f2f7ec2e-0b59-4ba1-86a3-c68334f0003b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200440, 'Main', 9200440);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200440, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200450, 'Blood Rally Point', 0, 500012, 7, 'fec03363-4dc1-4817-80e8-d2bb727208d5');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200450, 'Main', 9200450);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200450, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200460, 'Blood Refuge', 0, 500012, 7, 'e995a58f-359f-467b-86c0-d796d1bca367');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200460, 'Main', 9200460);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200460, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200470, 'Blood Sanctum', 0, 500012, 7, '97e84dfe-4fc1-4f7d-8d54-6e28eee16b9f');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200470, 'Main', 9200470);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200470, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200480, 'Blood Yard', 0, 500012, 7, 'b4ad83d1-ddc7-4671-83f6-72da86f2baf2');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200480, 'Main', 9200480);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200480, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200490, 'Crumbling Ruins', 0, 500022, 8, 'b09e5e62-ac96-4cf1-aa8d-3b1fa94c8828');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200490, 'Main', 9200490);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200490, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200500, 'Drone Assembly', 0, 500022, 7, '55bbda05-fe50-4db2-9941-fa65a8bec23d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200500, 'Main', 9200500);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200500, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200510, 'Drone Cluster', 0, 500022, 7, '50d5636d-2f92-4bf6-a313-2eecde7cb596');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200510, 'Main', 9200510);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200510, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200520, 'Drone Collection', 0, 500022, 7, '9bbf56e7-e7cb-453b-abb4-7480b6c3697c');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200520, 'Main', 9200520);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200520, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200530, 'Drone Gathering', 0, 500022, 7, 'b6d6cb19-0026-4fc4-8ae0-5bef04494ed4');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200530, 'Main', 9200530);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200530, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200540, 'Drone Herd', 0, 500022, 7, '20872247-65d0-4963-a492-3291388b6588');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200540, 'Main', 9200540);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200540, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200550, 'Drone Horde', 0, 500022, 7, 'f405eabb-b574-422a-8713-e6f8cbd03831');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200550, 'Main', 9200550);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200550, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200560, 'Infested Starbase', 0, 500022, 7, '17cb602a-587e-45ea-be6d-3aabd34851ac');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200560, 'Main', 9200560);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200560, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200570, 'Drone Menagerie', 0, 500022, 7, 'dab0d9f6-b5da-475f-82fd-e7211f3496bd');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200570, 'Main', 9200570);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200570, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200580, 'Drone Patrol', 0, 500022, 7, '4b5d4707-3438-45be-a92f-98d9346e710e');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200580, 'Main', 9200580);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200580, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200590, 'Drone Squad', 0, 500022, 7, '3046e636-6b44-4938-a58c-85190a78a0a9');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200590, 'Main', 9200590);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200590, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200600, 'Drone Surveillance', 0, 500022, 7, '8a98100c-5613-41d2-b2ac-5d6a35f60379');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200600, 'Main', 9200600);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200600, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200610, 'EDENCOM Field Base', 0, 500006, 7, '42898beb-8bca-45f5-81cb-4a49dc55722b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200610, 'Main', 9200610);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200610, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200620, 'EDENCOM Forward Post', 0, 500006, 7, 'cbc62a73-58bc-4632-8ca8-4d5fc74c6b31');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200620, 'Main', 9200620);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200620, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200630, 'EDENCOM Staging Area', 0, 500006, 7, '11cb705d-7bb9-43e2-8f4b-c7308dcd8a88');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200630, 'Main', 9200630);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200630, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200640, 'Emerging Conduit', 0, 500021, 7, '9ae35a05-4455-403b-8e34-5528c60640e0');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200640, 'Main', 9200640);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200640, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200650, 'Festering Ruins', 0, 500022, 8, '6db787a7-9d8b-4264-8454-0e1a8ee48f7b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200650, 'Main', 9200650);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200650, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200660, 'Forgotten Ruins', 0, 500022, 8, 'ddc85052-7e73-4890-891c-09f55137adff');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200660, 'Main', 9200660);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200660, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200670, 'Guristas Burrow', 0, 500010, 7, 'c38ab1ec-8db6-4237-ac6b-8a1baeede626');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200670, 'Main', 9200670);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200670, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200680, 'Guristas Den', 0, 500010, 7, '47325865-cd83-42ea-9300-5633c3d0dd5a');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200680, 'Main', 9200680);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200680, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200690, 'Guristas Forlorn Den', 0, 500010, 7, '7fcaf951-3d58-4820-96b7-4e4cba305e39');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200690, 'Main', 9200690);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200690, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200700, 'Guristas Forlorn Hideaway', 0, 500010, 7, '29160b38-0ac9-43d3-b82b-fc77c82ec2cf');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200700, 'Main', 9200700);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200700, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200710, 'Guristas Forlorn Hub', 0, 500010, 7, '4db9c018-45bc-4c03-8aaf-91ed74020730');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200710, 'Main', 9200710);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200710, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200720, 'Guristas Forlorn Rally Point', 0, 500010, 7, 'afb2c006-47a4-4500-8203-1aaeac194bf2');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200720, 'Main', 9200720);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200720, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200730, 'Guristas Forsaken Den', 0, 500010, 7, 'e962c909-41b7-4066-9b73-0634f12ae94b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200730, 'Main', 9200730);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200730, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200740, 'Guristas Forsaken Hideaway', 0, 500010, 7, '64cf69d8-bab6-4b9e-9875-dc57adc293e6');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200740, 'Main', 9200740);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200740, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200750, 'Guristas Forsaken Hub', 0, 500010, 7, '701465e3-dbdf-4c7b-9efc-73834bb6675b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200750, 'Main', 9200750);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200750, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200760, 'Guristas Forsaken Rally Point', 0, 500010, 7, '87b1f09a-57ac-4912-b664-49991d31a3a4');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200760, 'Main', 9200760);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200760, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200770, 'Guristas Forsaken Sanctum', 0, 500010, 7, '10e11dc6-fc9b-494b-9ab7-abea59026456');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200770, 'Main', 9200770);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200770, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200780, 'Guristas Haven', 0, 500010, 7, '88cae1a0-0a1a-4277-8edd-3e3bfee1964c');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200780, 'Main', 9200780);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200780, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200790, 'Guristas Hidden Den', 0, 500010, 7, 'd7db8712-7451-45fd-bed6-163e4439df0e');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200790, 'Main', 9200790);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200790, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200800, 'Guristas Hidden Hideaway', 0, 500010, 7, '1287bf19-ef10-4f30-b25f-af3be44651b6');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200800, 'Main', 9200800);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200800, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200810, 'Guristas Hidden Hub', 0, 500010, 7, '134d6d69-b04c-49c0-8410-cb9962af6a4f');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200810, 'Main', 9200810);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200810, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200820, 'Guristas Hidden Rally Point', 0, 500010, 7, '3ffae22e-a985-41a2-b415-45b3cb3ca621');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200820, 'Main', 9200820);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200820, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200830, 'Guristas Hideaway', 0, 500010, 7, '45a5235c-20bb-4f2a-b09c-63258147b5d0');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200830, 'Main', 9200830);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200830, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200840, 'Guristas Hub', 0, 500010, 7, '4b4a843d-d820-4ba3-8954-fc02c48a7aec');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200840, 'Main', 9200840);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200840, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200850, 'Guristas Port', 0, 500010, 7, '436488d3-be74-45b8-bb85-fefc9e250e09');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200850, 'Main', 9200850);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200850, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200860, 'Guristas Rally Point', 0, 500010, 7, '7a4e2cd3-a138-48d7-86bf-4af23b8965e8');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200860, 'Main', 9200860);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200860, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200870, 'Guristas Refuge', 0, 500010, 7, 'c07ad37e-7c3a-47d1-be41-7369862a6ab8');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200870, 'Main', 9200870);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200870, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200880, 'Guristas Sanctum', 0, 500010, 7, '7b3fb469-72fc-4507-a28d-d8047fa8586c');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200880, 'Main', 9200880);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200880, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200890, 'Guristas Yard', 0, 500010, 7, '2c0d88f1-814c-416d-9f51-8e5e2b29bee2');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200890, 'Main', 9200890);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200890, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200900, 'Hidden Ruins', 0, 500022, 8, 'bfba7d27-209e-440c-82a9-f3a60eeb2b0b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200900, 'Main', 9200900);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200900, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200910, 'Major Conduit', 0, 500021, 7, '88f075ce-ee4b-4fb4-8075-4dfcf75a28b6');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200910, 'Main', 9200910);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200910, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200920, 'Minor Conduit', 0, 500021, 7, '2c5ff8d6-1360-4f31-b6ac-65f214e9ab84');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200920, 'Main', 9200920);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200920, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200930, 'Observatory Flashpoint', 0, 500022, 7, '09785ddb-2e4a-4b11-b2b4-aeec5053b0f5');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200930, 'Main', 9200930);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200930, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200940, 'Sansha Burrow', 0, 500019, 7, '0fa44327-3881-4d99-957a-30531459c3d5');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200940, 'Main', 9200940);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200940, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200950, 'Sansha Den', 0, 500019, 7, '35df379a-c3ab-43cf-adab-16e4d6a69742');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200950, 'Main', 9200950);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200950, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200960, 'Sansha Forlorn Den', 0, 500019, 7, '631e26a8-fb0e-4830-9016-a2a7b266134a');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200960, 'Main', 9200960);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200960, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200970, 'Sansha Forlorn Hideaway', 0, 500019, 7, '4059b25d-71f1-494b-8440-f9d792c4ae85');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200970, 'Main', 9200970);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200970, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200980, 'Sansha Forlorn Hub', 0, 500019, 7, '6bdc70a9-261a-4177-9f13-d996ab7b2266');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200980, 'Main', 9200980);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200980, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9200990, 'Sansha Forlorn Rally Point', 0, 500019, 7, '5eccc2bf-33f5-4779-85b4-fd2ba1a66733');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19200990, 'Main', 9200990);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19200990, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201000, 'Sansha Forsaken Den', 0, 500019, 7, 'cc880371-8572-4864-ba5d-664dd9edccc5');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201000, 'Main', 9201000);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201000, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201010, 'Sansha Forsaken Hideaway', 0, 500019, 7, '4c83e948-e17b-4faa-abf0-7f8d179fbddd');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201010, 'Main', 9201010);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201010, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201020, 'Sansha Forsaken Hub', 0, 500019, 7, '3b63435f-cef8-44b4-a755-5f3b91b88526');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201020, 'Main', 9201020);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201020, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201030, 'Sansha Forsaken Rally Point', 0, 500019, 7, '4642cf6a-5f6d-49bc-9479-f35830322f7f');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201030, 'Main', 9201030);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201030, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201040, 'Sansha Forsaken Sanctum', 0, 500019, 7, '523eca0e-ea1e-48d1-88b6-71b01bc2b8f8');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201040, 'Main', 9201040);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201040, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201050, 'Sansha Haven', 0, 500019, 7, 'dc1c42c8-6f6c-493a-9123-5e8723b42231');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201050, 'Main', 9201050);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201050, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201060, 'Sansha Hidden Den', 0, 500019, 7, 'e8a842a7-6d8a-47a1-b4a8-b07919efcc27');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201060, 'Main', 9201060);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201060, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201070, 'Sansha Hidden Hideaway', 0, 500019, 7, '55cd0430-18b8-465a-8129-9b55fcb7b754');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201070, 'Main', 9201070);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201070, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201080, 'Sansha Hidden Hub', 0, 500019, 7, '4b62b0f2-2da7-46ce-b58a-1ae51de41d4a');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201080, 'Main', 9201080);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201080, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201090, 'Sansha Hidden Rally Point', 0, 500019, 7, 'dc10aba4-5565-49b3-b81e-b5655655fcc7');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201090, 'Main', 9201090);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201090, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201100, 'Sansha Hideaway', 0, 500019, 7, '176a3a83-721e-4777-9107-0fe53d318bce');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201100, 'Main', 9201100);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201100, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201110, 'Sansha Hub', 0, 500019, 7, '9eadd818-853a-44a5-9f80-fc2d2dfad456');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201110, 'Main', 9201110);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201110, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201120, 'Sansha Port', 0, 500019, 7, 'a29019b6-f3be-4295-a4cd-3a16b5aba3b4');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201120, 'Main', 9201120);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201120, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201130, 'Sansha Rally Point', 0, 500019, 7, '64acbb8d-4fbb-4c07-b864-0cb017b8fe7d');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201130, 'Main', 9201130);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201130, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201140, 'Sansha Refuge', 0, 500019, 7, 'a681b15e-8e35-454c-86f1-af24412c5907');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201140, 'Main', 9201140);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201140, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201150, 'Sansha Sanctum', 0, 500019, 7, 'f59ab220-ea7f-45cb-9cef-fc8e35968e5f');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201150, 'Main', 9201150);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201150, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201160, 'Sansha Yard', 0, 500019, 7, 'bde7ff26-38e9-4fb1-980f-8df94802d2ec');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201160, 'Main', 9201160);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201160, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201170, 'Serpentis Burrow', 0, 500020, 7, 'd7a2498b-2093-4c0d-9052-ae6f950ac8c3');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201170, 'Main', 9201170);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201170, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201180, 'Serpentis Den', 0, 500020, 7, '9b6e899c-48e2-433d-845d-615e00d4825b');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201180, 'Main', 9201180);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201180, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201190, 'Serpentis Forlorn Den', 0, 500020, 7, 'db007114-e353-434d-947d-22fde8124143');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201190, 'Main', 9201190);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201190, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201200, 'Serpentis Forlorn Hideaway', 0, 500020, 7, 'e242559d-31aa-4072-8e73-9db91f679a56');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201200, 'Main', 9201200);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201200, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201210, 'Serpentis Forlorn Hub', 0, 500020, 7, 'ccf87956-bc7b-44f0-84b3-684d8f183b8f');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201210, 'Main', 9201210);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201210, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201220, 'Serpentis Forlorn Rally Point', 0, 500020, 7, 'b2015f04-1703-4da0-a72a-74442fcd85c3');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201220, 'Main', 9201220);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201220, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201230, 'Serpentis Forsaken Den', 0, 500020, 7, '7b947219-16f8-4e1b-82ad-e91ee2610bdc');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201230, 'Main', 9201230);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201230, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201240, 'Serpentis Forsaken Hideaway', 0, 500020, 7, '6082cb32-90ab-4fa6-b9fa-a0c356cdec44');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201240, 'Main', 9201240);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201240, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201250, 'Serpentis Forsaken Hub', 0, 500020, 7, '6fa4838e-26c9-4368-b4b9-142bcb49c094');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201250, 'Main', 9201250);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201250, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201260, 'Serpentis Forsaken Rally Point', 0, 500020, 7, '375efd3d-7669-4616-972d-692250f32993');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201260, 'Main', 9201260);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201260, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201270, 'Serpentis Forsaken Sanctum', 0, 500020, 7, 'c6e9f9f4-85fe-47df-a7d1-50521660fadf');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201270, 'Main', 9201270);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201270, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201280, 'Serpentis Haven', 0, 500020, 7, 'efced573-cdbe-4279-8f88-15be6a533bb4');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201280, 'Main', 9201280);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201280, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201290, 'Serpentis Hidden Den', 0, 500020, 7, '40449ca4-f779-4b63-b580-903c81e46fe0');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201290, 'Main', 9201290);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201290, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201300, 'Serpentis Hidden Hideaway', 0, 500020, 7, '019070e9-a93c-447f-9555-96770baffec8');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201300, 'Main', 9201300);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201300, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201310, 'Serpentis Hidden Hub', 0, 500020, 7, 'eb8d8307-2c17-4d00-a121-13e376f0a033');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201310, 'Main', 9201310);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201310, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201320, 'Serpentis Hidden Rally Point', 0, 500020, 7, 'a541d9f5-9eec-4462-ae0e-a3925e20f2cd');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201320, 'Main', 9201320);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201320, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201330, 'Serpentis Hideaway', 0, 500020, 7, 'b36797b9-e77f-4850-a707-0b44e1858a52');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201330, 'Main', 9201330);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201330, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201340, 'Serpentis Hub', 0, 500020, 7, 'b4bbcfbb-4827-4d3b-ad43-829c6fbd6add');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201340, 'Main', 9201340);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201340, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201350, 'Serpentis Port', 0, 500020, 7, 'a901596c-2cb1-4d76-b896-f42c11aec046');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201350, 'Main', 9201350);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201350, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201360, 'Serpentis Rally Point', 0, 500020, 7, '1e8357df-9ed6-4adf-bf8a-5eb6e13a9318');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201360, 'Main', 9201360);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201360, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201370, 'Serpentis Refuge', 0, 500020, 7, '05507f08-a6b9-4172-9550-077f3b142ef8');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201370, 'Main', 9201370);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201370, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201380, 'Serpentis Sanctum', 0, 500020, 7, '59e7714e-f183-43cf-ab2b-a7931f442ff6');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201380, 'Main', 9201380);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201380, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201390, 'Shielded Starbase', 0, 500020, 7, 'aa6e24a5-0e3e-4e4f-ab4f-9d4a90e65d05');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201390, 'Main', 9201390);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201390, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201400, 'Serpentis Yard', 0, 500020, 7, '0754d8b1-6e0f-408e-a499-80d50b9c0a24');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201400, 'Main', 9201400);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201400, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201410, 'Stellar Fleet Deployment Site', 0, 500022, 7, '55b4458b-060f-4f54-a00b-6a2c24a989d1');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201410, 'Main', 9201410);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201410, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201420, 'security status', 0, 500022, 7, 'c1e4e085-cb62-4ef0-9732-3cee1a8ebf2a');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201420, 'Main', 9201420);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201420, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201430, 'Teeming Drone Horde', 0, 500022, 7, '98c6757d-32ec-4ca7-a3e1-56dc8411ee02');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201430, 'Main', 9201430);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201430, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201440, 'Wispy Ruins', 0, 500022, 8, '198c0c24-7ab0-4337-9c9c-01a83d55c031');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201440, 'Main', 9201440);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201440, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) VALUES (9201450, 'World Ark Deployment Site', 0, 500022, 7, '617af3b5-70e9-4367-8cda-f26dbc1a945c');
INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES (19201450, 'Main', 9201450);
INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) SELECT 19201450, 25829, groupID, 0.0, 0.0, 0.0, 0, 0, 0, COALESCE(radius, 500) FROM invTypes WHERE typeID = 25829 LIMIT 1;

-- +migrate Down
DELETE FROM dunRoomObjects WHERE roomID = 19201450;
DELETE FROM dunRooms WHERE dungeonID = 9201450;
DELETE FROM dunDungeons WHERE dungeonID = 9201450;
DELETE FROM dunRoomObjects WHERE roomID = 19201440;
DELETE FROM dunRooms WHERE dungeonID = 9201440;
DELETE FROM dunDungeons WHERE dungeonID = 9201440;
DELETE FROM dunRoomObjects WHERE roomID = 19201430;
DELETE FROM dunRooms WHERE dungeonID = 9201430;
DELETE FROM dunDungeons WHERE dungeonID = 9201430;
DELETE FROM dunRoomObjects WHERE roomID = 19201420;
DELETE FROM dunRooms WHERE dungeonID = 9201420;
DELETE FROM dunDungeons WHERE dungeonID = 9201420;
DELETE FROM dunRoomObjects WHERE roomID = 19201410;
DELETE FROM dunRooms WHERE dungeonID = 9201410;
DELETE FROM dunDungeons WHERE dungeonID = 9201410;
DELETE FROM dunRoomObjects WHERE roomID = 19201400;
DELETE FROM dunRooms WHERE dungeonID = 9201400;
DELETE FROM dunDungeons WHERE dungeonID = 9201400;
DELETE FROM dunRoomObjects WHERE roomID = 19201390;
DELETE FROM dunRooms WHERE dungeonID = 9201390;
DELETE FROM dunDungeons WHERE dungeonID = 9201390;
DELETE FROM dunRoomObjects WHERE roomID = 19201380;
DELETE FROM dunRooms WHERE dungeonID = 9201380;
DELETE FROM dunDungeons WHERE dungeonID = 9201380;
DELETE FROM dunRoomObjects WHERE roomID = 19201370;
DELETE FROM dunRooms WHERE dungeonID = 9201370;
DELETE FROM dunDungeons WHERE dungeonID = 9201370;
DELETE FROM dunRoomObjects WHERE roomID = 19201360;
DELETE FROM dunRooms WHERE dungeonID = 9201360;
DELETE FROM dunDungeons WHERE dungeonID = 9201360;
DELETE FROM dunRoomObjects WHERE roomID = 19201350;
DELETE FROM dunRooms WHERE dungeonID = 9201350;
DELETE FROM dunDungeons WHERE dungeonID = 9201350;
DELETE FROM dunRoomObjects WHERE roomID = 19201340;
DELETE FROM dunRooms WHERE dungeonID = 9201340;
DELETE FROM dunDungeons WHERE dungeonID = 9201340;
DELETE FROM dunRoomObjects WHERE roomID = 19201330;
DELETE FROM dunRooms WHERE dungeonID = 9201330;
DELETE FROM dunDungeons WHERE dungeonID = 9201330;
DELETE FROM dunRoomObjects WHERE roomID = 19201320;
DELETE FROM dunRooms WHERE dungeonID = 9201320;
DELETE FROM dunDungeons WHERE dungeonID = 9201320;
DELETE FROM dunRoomObjects WHERE roomID = 19201310;
DELETE FROM dunRooms WHERE dungeonID = 9201310;
DELETE FROM dunDungeons WHERE dungeonID = 9201310;
DELETE FROM dunRoomObjects WHERE roomID = 19201300;
DELETE FROM dunRooms WHERE dungeonID = 9201300;
DELETE FROM dunDungeons WHERE dungeonID = 9201300;
DELETE FROM dunRoomObjects WHERE roomID = 19201290;
DELETE FROM dunRooms WHERE dungeonID = 9201290;
DELETE FROM dunDungeons WHERE dungeonID = 9201290;
DELETE FROM dunRoomObjects WHERE roomID = 19201280;
DELETE FROM dunRooms WHERE dungeonID = 9201280;
DELETE FROM dunDungeons WHERE dungeonID = 9201280;
DELETE FROM dunRoomObjects WHERE roomID = 19201270;
DELETE FROM dunRooms WHERE dungeonID = 9201270;
DELETE FROM dunDungeons WHERE dungeonID = 9201270;
DELETE FROM dunRoomObjects WHERE roomID = 19201260;
DELETE FROM dunRooms WHERE dungeonID = 9201260;
DELETE FROM dunDungeons WHERE dungeonID = 9201260;
DELETE FROM dunRoomObjects WHERE roomID = 19201250;
DELETE FROM dunRooms WHERE dungeonID = 9201250;
DELETE FROM dunDungeons WHERE dungeonID = 9201250;
DELETE FROM dunRoomObjects WHERE roomID = 19201240;
DELETE FROM dunRooms WHERE dungeonID = 9201240;
DELETE FROM dunDungeons WHERE dungeonID = 9201240;
DELETE FROM dunRoomObjects WHERE roomID = 19201230;
DELETE FROM dunRooms WHERE dungeonID = 9201230;
DELETE FROM dunDungeons WHERE dungeonID = 9201230;
DELETE FROM dunRoomObjects WHERE roomID = 19201220;
DELETE FROM dunRooms WHERE dungeonID = 9201220;
DELETE FROM dunDungeons WHERE dungeonID = 9201220;
DELETE FROM dunRoomObjects WHERE roomID = 19201210;
DELETE FROM dunRooms WHERE dungeonID = 9201210;
DELETE FROM dunDungeons WHERE dungeonID = 9201210;
DELETE FROM dunRoomObjects WHERE roomID = 19201200;
DELETE FROM dunRooms WHERE dungeonID = 9201200;
DELETE FROM dunDungeons WHERE dungeonID = 9201200;
DELETE FROM dunRoomObjects WHERE roomID = 19201190;
DELETE FROM dunRooms WHERE dungeonID = 9201190;
DELETE FROM dunDungeons WHERE dungeonID = 9201190;
DELETE FROM dunRoomObjects WHERE roomID = 19201180;
DELETE FROM dunRooms WHERE dungeonID = 9201180;
DELETE FROM dunDungeons WHERE dungeonID = 9201180;
DELETE FROM dunRoomObjects WHERE roomID = 19201170;
DELETE FROM dunRooms WHERE dungeonID = 9201170;
DELETE FROM dunDungeons WHERE dungeonID = 9201170;
DELETE FROM dunRoomObjects WHERE roomID = 19201160;
DELETE FROM dunRooms WHERE dungeonID = 9201160;
DELETE FROM dunDungeons WHERE dungeonID = 9201160;
DELETE FROM dunRoomObjects WHERE roomID = 19201150;
DELETE FROM dunRooms WHERE dungeonID = 9201150;
DELETE FROM dunDungeons WHERE dungeonID = 9201150;
DELETE FROM dunRoomObjects WHERE roomID = 19201140;
DELETE FROM dunRooms WHERE dungeonID = 9201140;
DELETE FROM dunDungeons WHERE dungeonID = 9201140;
DELETE FROM dunRoomObjects WHERE roomID = 19201130;
DELETE FROM dunRooms WHERE dungeonID = 9201130;
DELETE FROM dunDungeons WHERE dungeonID = 9201130;
DELETE FROM dunRoomObjects WHERE roomID = 19201120;
DELETE FROM dunRooms WHERE dungeonID = 9201120;
DELETE FROM dunDungeons WHERE dungeonID = 9201120;
DELETE FROM dunRoomObjects WHERE roomID = 19201110;
DELETE FROM dunRooms WHERE dungeonID = 9201110;
DELETE FROM dunDungeons WHERE dungeonID = 9201110;
DELETE FROM dunRoomObjects WHERE roomID = 19201100;
DELETE FROM dunRooms WHERE dungeonID = 9201100;
DELETE FROM dunDungeons WHERE dungeonID = 9201100;
DELETE FROM dunRoomObjects WHERE roomID = 19201090;
DELETE FROM dunRooms WHERE dungeonID = 9201090;
DELETE FROM dunDungeons WHERE dungeonID = 9201090;
DELETE FROM dunRoomObjects WHERE roomID = 19201080;
DELETE FROM dunRooms WHERE dungeonID = 9201080;
DELETE FROM dunDungeons WHERE dungeonID = 9201080;
DELETE FROM dunRoomObjects WHERE roomID = 19201070;
DELETE FROM dunRooms WHERE dungeonID = 9201070;
DELETE FROM dunDungeons WHERE dungeonID = 9201070;
DELETE FROM dunRoomObjects WHERE roomID = 19201060;
DELETE FROM dunRooms WHERE dungeonID = 9201060;
DELETE FROM dunDungeons WHERE dungeonID = 9201060;
DELETE FROM dunRoomObjects WHERE roomID = 19201050;
DELETE FROM dunRooms WHERE dungeonID = 9201050;
DELETE FROM dunDungeons WHERE dungeonID = 9201050;
DELETE FROM dunRoomObjects WHERE roomID = 19201040;
DELETE FROM dunRooms WHERE dungeonID = 9201040;
DELETE FROM dunDungeons WHERE dungeonID = 9201040;
DELETE FROM dunRoomObjects WHERE roomID = 19201030;
DELETE FROM dunRooms WHERE dungeonID = 9201030;
DELETE FROM dunDungeons WHERE dungeonID = 9201030;
DELETE FROM dunRoomObjects WHERE roomID = 19201020;
DELETE FROM dunRooms WHERE dungeonID = 9201020;
DELETE FROM dunDungeons WHERE dungeonID = 9201020;
DELETE FROM dunRoomObjects WHERE roomID = 19201010;
DELETE FROM dunRooms WHERE dungeonID = 9201010;
DELETE FROM dunDungeons WHERE dungeonID = 9201010;
DELETE FROM dunRoomObjects WHERE roomID = 19201000;
DELETE FROM dunRooms WHERE dungeonID = 9201000;
DELETE FROM dunDungeons WHERE dungeonID = 9201000;
DELETE FROM dunRoomObjects WHERE roomID = 19200990;
DELETE FROM dunRooms WHERE dungeonID = 9200990;
DELETE FROM dunDungeons WHERE dungeonID = 9200990;
DELETE FROM dunRoomObjects WHERE roomID = 19200980;
DELETE FROM dunRooms WHERE dungeonID = 9200980;
DELETE FROM dunDungeons WHERE dungeonID = 9200980;
DELETE FROM dunRoomObjects WHERE roomID = 19200970;
DELETE FROM dunRooms WHERE dungeonID = 9200970;
DELETE FROM dunDungeons WHERE dungeonID = 9200970;
DELETE FROM dunRoomObjects WHERE roomID = 19200960;
DELETE FROM dunRooms WHERE dungeonID = 9200960;
DELETE FROM dunDungeons WHERE dungeonID = 9200960;
DELETE FROM dunRoomObjects WHERE roomID = 19200950;
DELETE FROM dunRooms WHERE dungeonID = 9200950;
DELETE FROM dunDungeons WHERE dungeonID = 9200950;
DELETE FROM dunRoomObjects WHERE roomID = 19200940;
DELETE FROM dunRooms WHERE dungeonID = 9200940;
DELETE FROM dunDungeons WHERE dungeonID = 9200940;
DELETE FROM dunRoomObjects WHERE roomID = 19200930;
DELETE FROM dunRooms WHERE dungeonID = 9200930;
DELETE FROM dunDungeons WHERE dungeonID = 9200930;
DELETE FROM dunRoomObjects WHERE roomID = 19200920;
DELETE FROM dunRooms WHERE dungeonID = 9200920;
DELETE FROM dunDungeons WHERE dungeonID = 9200920;
DELETE FROM dunRoomObjects WHERE roomID = 19200910;
DELETE FROM dunRooms WHERE dungeonID = 9200910;
DELETE FROM dunDungeons WHERE dungeonID = 9200910;
DELETE FROM dunRoomObjects WHERE roomID = 19200900;
DELETE FROM dunRooms WHERE dungeonID = 9200900;
DELETE FROM dunDungeons WHERE dungeonID = 9200900;
DELETE FROM dunRoomObjects WHERE roomID = 19200890;
DELETE FROM dunRooms WHERE dungeonID = 9200890;
DELETE FROM dunDungeons WHERE dungeonID = 9200890;
DELETE FROM dunRoomObjects WHERE roomID = 19200880;
DELETE FROM dunRooms WHERE dungeonID = 9200880;
DELETE FROM dunDungeons WHERE dungeonID = 9200880;
DELETE FROM dunRoomObjects WHERE roomID = 19200870;
DELETE FROM dunRooms WHERE dungeonID = 9200870;
DELETE FROM dunDungeons WHERE dungeonID = 9200870;
DELETE FROM dunRoomObjects WHERE roomID = 19200860;
DELETE FROM dunRooms WHERE dungeonID = 9200860;
DELETE FROM dunDungeons WHERE dungeonID = 9200860;
DELETE FROM dunRoomObjects WHERE roomID = 19200850;
DELETE FROM dunRooms WHERE dungeonID = 9200850;
DELETE FROM dunDungeons WHERE dungeonID = 9200850;
DELETE FROM dunRoomObjects WHERE roomID = 19200840;
DELETE FROM dunRooms WHERE dungeonID = 9200840;
DELETE FROM dunDungeons WHERE dungeonID = 9200840;
DELETE FROM dunRoomObjects WHERE roomID = 19200830;
DELETE FROM dunRooms WHERE dungeonID = 9200830;
DELETE FROM dunDungeons WHERE dungeonID = 9200830;
DELETE FROM dunRoomObjects WHERE roomID = 19200820;
DELETE FROM dunRooms WHERE dungeonID = 9200820;
DELETE FROM dunDungeons WHERE dungeonID = 9200820;
DELETE FROM dunRoomObjects WHERE roomID = 19200810;
DELETE FROM dunRooms WHERE dungeonID = 9200810;
DELETE FROM dunDungeons WHERE dungeonID = 9200810;
DELETE FROM dunRoomObjects WHERE roomID = 19200800;
DELETE FROM dunRooms WHERE dungeonID = 9200800;
DELETE FROM dunDungeons WHERE dungeonID = 9200800;
DELETE FROM dunRoomObjects WHERE roomID = 19200790;
DELETE FROM dunRooms WHERE dungeonID = 9200790;
DELETE FROM dunDungeons WHERE dungeonID = 9200790;
DELETE FROM dunRoomObjects WHERE roomID = 19200780;
DELETE FROM dunRooms WHERE dungeonID = 9200780;
DELETE FROM dunDungeons WHERE dungeonID = 9200780;
DELETE FROM dunRoomObjects WHERE roomID = 19200770;
DELETE FROM dunRooms WHERE dungeonID = 9200770;
DELETE FROM dunDungeons WHERE dungeonID = 9200770;
DELETE FROM dunRoomObjects WHERE roomID = 19200760;
DELETE FROM dunRooms WHERE dungeonID = 9200760;
DELETE FROM dunDungeons WHERE dungeonID = 9200760;
DELETE FROM dunRoomObjects WHERE roomID = 19200750;
DELETE FROM dunRooms WHERE dungeonID = 9200750;
DELETE FROM dunDungeons WHERE dungeonID = 9200750;
DELETE FROM dunRoomObjects WHERE roomID = 19200740;
DELETE FROM dunRooms WHERE dungeonID = 9200740;
DELETE FROM dunDungeons WHERE dungeonID = 9200740;
DELETE FROM dunRoomObjects WHERE roomID = 19200730;
DELETE FROM dunRooms WHERE dungeonID = 9200730;
DELETE FROM dunDungeons WHERE dungeonID = 9200730;
DELETE FROM dunRoomObjects WHERE roomID = 19200720;
DELETE FROM dunRooms WHERE dungeonID = 9200720;
DELETE FROM dunDungeons WHERE dungeonID = 9200720;
DELETE FROM dunRoomObjects WHERE roomID = 19200710;
DELETE FROM dunRooms WHERE dungeonID = 9200710;
DELETE FROM dunDungeons WHERE dungeonID = 9200710;
DELETE FROM dunRoomObjects WHERE roomID = 19200700;
DELETE FROM dunRooms WHERE dungeonID = 9200700;
DELETE FROM dunDungeons WHERE dungeonID = 9200700;
DELETE FROM dunRoomObjects WHERE roomID = 19200690;
DELETE FROM dunRooms WHERE dungeonID = 9200690;
DELETE FROM dunDungeons WHERE dungeonID = 9200690;
DELETE FROM dunRoomObjects WHERE roomID = 19200680;
DELETE FROM dunRooms WHERE dungeonID = 9200680;
DELETE FROM dunDungeons WHERE dungeonID = 9200680;
DELETE FROM dunRoomObjects WHERE roomID = 19200670;
DELETE FROM dunRooms WHERE dungeonID = 9200670;
DELETE FROM dunDungeons WHERE dungeonID = 9200670;
DELETE FROM dunRoomObjects WHERE roomID = 19200660;
DELETE FROM dunRooms WHERE dungeonID = 9200660;
DELETE FROM dunDungeons WHERE dungeonID = 9200660;
DELETE FROM dunRoomObjects WHERE roomID = 19200650;
DELETE FROM dunRooms WHERE dungeonID = 9200650;
DELETE FROM dunDungeons WHERE dungeonID = 9200650;
DELETE FROM dunRoomObjects WHERE roomID = 19200640;
DELETE FROM dunRooms WHERE dungeonID = 9200640;
DELETE FROM dunDungeons WHERE dungeonID = 9200640;
DELETE FROM dunRoomObjects WHERE roomID = 19200630;
DELETE FROM dunRooms WHERE dungeonID = 9200630;
DELETE FROM dunDungeons WHERE dungeonID = 9200630;
DELETE FROM dunRoomObjects WHERE roomID = 19200620;
DELETE FROM dunRooms WHERE dungeonID = 9200620;
DELETE FROM dunDungeons WHERE dungeonID = 9200620;
DELETE FROM dunRoomObjects WHERE roomID = 19200610;
DELETE FROM dunRooms WHERE dungeonID = 9200610;
DELETE FROM dunDungeons WHERE dungeonID = 9200610;
DELETE FROM dunRoomObjects WHERE roomID = 19200600;
DELETE FROM dunRooms WHERE dungeonID = 9200600;
DELETE FROM dunDungeons WHERE dungeonID = 9200600;
DELETE FROM dunRoomObjects WHERE roomID = 19200590;
DELETE FROM dunRooms WHERE dungeonID = 9200590;
DELETE FROM dunDungeons WHERE dungeonID = 9200590;
DELETE FROM dunRoomObjects WHERE roomID = 19200580;
DELETE FROM dunRooms WHERE dungeonID = 9200580;
DELETE FROM dunDungeons WHERE dungeonID = 9200580;
DELETE FROM dunRoomObjects WHERE roomID = 19200570;
DELETE FROM dunRooms WHERE dungeonID = 9200570;
DELETE FROM dunDungeons WHERE dungeonID = 9200570;
DELETE FROM dunRoomObjects WHERE roomID = 19200560;
DELETE FROM dunRooms WHERE dungeonID = 9200560;
DELETE FROM dunDungeons WHERE dungeonID = 9200560;
DELETE FROM dunRoomObjects WHERE roomID = 19200550;
DELETE FROM dunRooms WHERE dungeonID = 9200550;
DELETE FROM dunDungeons WHERE dungeonID = 9200550;
DELETE FROM dunRoomObjects WHERE roomID = 19200540;
DELETE FROM dunRooms WHERE dungeonID = 9200540;
DELETE FROM dunDungeons WHERE dungeonID = 9200540;
DELETE FROM dunRoomObjects WHERE roomID = 19200530;
DELETE FROM dunRooms WHERE dungeonID = 9200530;
DELETE FROM dunDungeons WHERE dungeonID = 9200530;
DELETE FROM dunRoomObjects WHERE roomID = 19200520;
DELETE FROM dunRooms WHERE dungeonID = 9200520;
DELETE FROM dunDungeons WHERE dungeonID = 9200520;
DELETE FROM dunRoomObjects WHERE roomID = 19200510;
DELETE FROM dunRooms WHERE dungeonID = 9200510;
DELETE FROM dunDungeons WHERE dungeonID = 9200510;
DELETE FROM dunRoomObjects WHERE roomID = 19200500;
DELETE FROM dunRooms WHERE dungeonID = 9200500;
DELETE FROM dunDungeons WHERE dungeonID = 9200500;
DELETE FROM dunRoomObjects WHERE roomID = 19200490;
DELETE FROM dunRooms WHERE dungeonID = 9200490;
DELETE FROM dunDungeons WHERE dungeonID = 9200490;
DELETE FROM dunRoomObjects WHERE roomID = 19200480;
DELETE FROM dunRooms WHERE dungeonID = 9200480;
DELETE FROM dunDungeons WHERE dungeonID = 9200480;
DELETE FROM dunRoomObjects WHERE roomID = 19200470;
DELETE FROM dunRooms WHERE dungeonID = 9200470;
DELETE FROM dunDungeons WHERE dungeonID = 9200470;
DELETE FROM dunRoomObjects WHERE roomID = 19200460;
DELETE FROM dunRooms WHERE dungeonID = 9200460;
DELETE FROM dunDungeons WHERE dungeonID = 9200460;
DELETE FROM dunRoomObjects WHERE roomID = 19200450;
DELETE FROM dunRooms WHERE dungeonID = 9200450;
DELETE FROM dunDungeons WHERE dungeonID = 9200450;
DELETE FROM dunRoomObjects WHERE roomID = 19200440;
DELETE FROM dunRooms WHERE dungeonID = 9200440;
DELETE FROM dunDungeons WHERE dungeonID = 9200440;
DELETE FROM dunRoomObjects WHERE roomID = 19200430;
DELETE FROM dunRooms WHERE dungeonID = 9200430;
DELETE FROM dunDungeons WHERE dungeonID = 9200430;
DELETE FROM dunRoomObjects WHERE roomID = 19200420;
DELETE FROM dunRooms WHERE dungeonID = 9200420;
DELETE FROM dunDungeons WHERE dungeonID = 9200420;
DELETE FROM dunRoomObjects WHERE roomID = 19200410;
DELETE FROM dunRooms WHERE dungeonID = 9200410;
DELETE FROM dunDungeons WHERE dungeonID = 9200410;
DELETE FROM dunRoomObjects WHERE roomID = 19200400;
DELETE FROM dunRooms WHERE dungeonID = 9200400;
DELETE FROM dunDungeons WHERE dungeonID = 9200400;
DELETE FROM dunRoomObjects WHERE roomID = 19200390;
DELETE FROM dunRooms WHERE dungeonID = 9200390;
DELETE FROM dunDungeons WHERE dungeonID = 9200390;
DELETE FROM dunRoomObjects WHERE roomID = 19200380;
DELETE FROM dunRooms WHERE dungeonID = 9200380;
DELETE FROM dunDungeons WHERE dungeonID = 9200380;
DELETE FROM dunRoomObjects WHERE roomID = 19200370;
DELETE FROM dunRooms WHERE dungeonID = 9200370;
DELETE FROM dunDungeons WHERE dungeonID = 9200370;
DELETE FROM dunRoomObjects WHERE roomID = 19200360;
DELETE FROM dunRooms WHERE dungeonID = 9200360;
DELETE FROM dunDungeons WHERE dungeonID = 9200360;
DELETE FROM dunRoomObjects WHERE roomID = 19200350;
DELETE FROM dunRooms WHERE dungeonID = 9200350;
DELETE FROM dunDungeons WHERE dungeonID = 9200350;
DELETE FROM dunRoomObjects WHERE roomID = 19200340;
DELETE FROM dunRooms WHERE dungeonID = 9200340;
DELETE FROM dunDungeons WHERE dungeonID = 9200340;
DELETE FROM dunRoomObjects WHERE roomID = 19200330;
DELETE FROM dunRooms WHERE dungeonID = 9200330;
DELETE FROM dunDungeons WHERE dungeonID = 9200330;
DELETE FROM dunRoomObjects WHERE roomID = 19200320;
DELETE FROM dunRooms WHERE dungeonID = 9200320;
DELETE FROM dunDungeons WHERE dungeonID = 9200320;
DELETE FROM dunRoomObjects WHERE roomID = 19200310;
DELETE FROM dunRooms WHERE dungeonID = 9200310;
DELETE FROM dunDungeons WHERE dungeonID = 9200310;
DELETE FROM dunRoomObjects WHERE roomID = 19200300;
DELETE FROM dunRooms WHERE dungeonID = 9200300;
DELETE FROM dunDungeons WHERE dungeonID = 9200300;
DELETE FROM dunRoomObjects WHERE roomID = 19200290;
DELETE FROM dunRooms WHERE dungeonID = 9200290;
DELETE FROM dunDungeons WHERE dungeonID = 9200290;
DELETE FROM dunRoomObjects WHERE roomID = 19200280;
DELETE FROM dunRooms WHERE dungeonID = 9200280;
DELETE FROM dunDungeons WHERE dungeonID = 9200280;
DELETE FROM dunRoomObjects WHERE roomID = 19200270;
DELETE FROM dunRooms WHERE dungeonID = 9200270;
DELETE FROM dunDungeons WHERE dungeonID = 9200270;
DELETE FROM dunRoomObjects WHERE roomID = 19200260;
DELETE FROM dunRooms WHERE dungeonID = 9200260;
DELETE FROM dunDungeons WHERE dungeonID = 9200260;
DELETE FROM dunRoomObjects WHERE roomID = 19200250;
DELETE FROM dunRooms WHERE dungeonID = 9200250;
DELETE FROM dunDungeons WHERE dungeonID = 9200250;
DELETE FROM dunRoomObjects WHERE roomID = 19200240;
DELETE FROM dunRooms WHERE dungeonID = 9200240;
DELETE FROM dunDungeons WHERE dungeonID = 9200240;
DELETE FROM dunRoomObjects WHERE roomID = 19200230;
DELETE FROM dunRooms WHERE dungeonID = 9200230;
DELETE FROM dunDungeons WHERE dungeonID = 9200230;
DELETE FROM dunRoomObjects WHERE roomID = 19200220;
DELETE FROM dunRooms WHERE dungeonID = 9200220;
DELETE FROM dunDungeons WHERE dungeonID = 9200220;
DELETE FROM dunRoomObjects WHERE roomID = 19200210;
DELETE FROM dunRooms WHERE dungeonID = 9200210;
DELETE FROM dunDungeons WHERE dungeonID = 9200210;
DELETE FROM dunRoomObjects WHERE roomID = 19200200;
DELETE FROM dunRooms WHERE dungeonID = 9200200;
DELETE FROM dunDungeons WHERE dungeonID = 9200200;
DELETE FROM dunRoomObjects WHERE roomID = 19200190;
DELETE FROM dunRooms WHERE dungeonID = 9200190;
DELETE FROM dunDungeons WHERE dungeonID = 9200190;
DELETE FROM dunRoomObjects WHERE roomID = 19200180;
DELETE FROM dunRooms WHERE dungeonID = 9200180;
DELETE FROM dunDungeons WHERE dungeonID = 9200180;
DELETE FROM dunRoomObjects WHERE roomID = 19200170;
DELETE FROM dunRooms WHERE dungeonID = 9200170;
DELETE FROM dunDungeons WHERE dungeonID = 9200170;
DELETE FROM dunRoomObjects WHERE roomID = 19200160;
DELETE FROM dunRooms WHERE dungeonID = 9200160;
DELETE FROM dunDungeons WHERE dungeonID = 9200160;
DELETE FROM dunRoomObjects WHERE roomID = 19200150;
DELETE FROM dunRooms WHERE dungeonID = 9200150;
DELETE FROM dunDungeons WHERE dungeonID = 9200150;
DELETE FROM dunRoomObjects WHERE roomID = 19200140;
DELETE FROM dunRooms WHERE dungeonID = 9200140;
DELETE FROM dunDungeons WHERE dungeonID = 9200140;
DELETE FROM dunRoomObjects WHERE roomID = 19200130;
DELETE FROM dunRooms WHERE dungeonID = 9200130;
DELETE FROM dunDungeons WHERE dungeonID = 9200130;
DELETE FROM dunRoomObjects WHERE roomID = 19200120;
DELETE FROM dunRooms WHERE dungeonID = 9200120;
DELETE FROM dunDungeons WHERE dungeonID = 9200120;
DELETE FROM dunRoomObjects WHERE roomID = 19200110;
DELETE FROM dunRooms WHERE dungeonID = 9200110;
DELETE FROM dunDungeons WHERE dungeonID = 9200110;
DELETE FROM dunRoomObjects WHERE roomID = 19200100;
DELETE FROM dunRooms WHERE dungeonID = 9200100;
DELETE FROM dunDungeons WHERE dungeonID = 9200100;
DELETE FROM dunRoomObjects WHERE roomID = 19200090;
DELETE FROM dunRooms WHERE dungeonID = 9200090;
DELETE FROM dunDungeons WHERE dungeonID = 9200090;
DELETE FROM dunRoomObjects WHERE roomID = 19200080;
DELETE FROM dunRooms WHERE dungeonID = 9200080;
DELETE FROM dunDungeons WHERE dungeonID = 9200080;
DELETE FROM dunRoomObjects WHERE roomID = 19200070;
DELETE FROM dunRooms WHERE dungeonID = 9200070;
DELETE FROM dunDungeons WHERE dungeonID = 9200070;
DELETE FROM dunRoomObjects WHERE roomID = 19200060;
DELETE FROM dunRooms WHERE dungeonID = 9200060;
DELETE FROM dunDungeons WHERE dungeonID = 9200060;
DELETE FROM dunRoomObjects WHERE roomID = 19200050;
DELETE FROM dunRooms WHERE dungeonID = 9200050;
DELETE FROM dunDungeons WHERE dungeonID = 9200050;
DELETE FROM dunRoomObjects WHERE roomID = 19200040;
DELETE FROM dunRooms WHERE dungeonID = 9200040;
DELETE FROM dunDungeons WHERE dungeonID = 9200040;
DELETE FROM dunRoomObjects WHERE roomID = 19200030;
DELETE FROM dunRooms WHERE dungeonID = 9200030;
DELETE FROM dunDungeons WHERE dungeonID = 9200030;
DELETE FROM dunRoomObjects WHERE roomID = 19200020;
DELETE FROM dunRooms WHERE dungeonID = 9200020;
DELETE FROM dunDungeons WHERE dungeonID = 9200020;
DELETE FROM dunRoomObjects WHERE roomID = 19200010;
DELETE FROM dunRooms WHERE dungeonID = 9200010;
DELETE FROM dunDungeons WHERE dungeonID = 9200010;
DELETE FROM dunRoomObjects WHERE roomID = 19200000;
DELETE FROM dunRooms WHERE dungeonID = 9200000;
DELETE FROM dunDungeons WHERE dungeonID = 9200000;
