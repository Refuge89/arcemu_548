DROP TABLE IF EXISTS `realm_classes`;

CREATE TABLE `realm_classes` (
  `realmId` INT(11) NOT NULL,
  `class` TINYINT(4) NOT NULL COMMENT 'Class Id',
  `expansion` TINYINT(4) NOT NULL COMMENT 'Expansion for class activation',
  PRIMARY KEY (`realmId`,`class`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

INSERT INTO realm_classes VALUES ('1', '1', '0');
INSERT INTO realm_classes VALUES ('1', '2', '0');
INSERT INTO realm_classes VALUES ('1', '3', '0');
INSERT INTO realm_classes VALUES ('1', '4', '0');
INSERT INTO realm_classes VALUES ('1', '5', '0');
INSERT INTO realm_classes VALUES ('1', '6', '2');
INSERT INTO realm_classes VALUES ('1', '7', '0');
INSERT INTO realm_classes VALUES ('1', '8', '0');
INSERT INTO realm_classes VALUES ('1', '9', '0');
INSERT INTO realm_classes VALUES ('1', '10', '4');
INSERT INTO realm_classes VALUES ('1', '11', '0');

DROP TABLE IF EXISTS `realm_races`;
CREATE TABLE `realm_races` (
  `realmId` INT(11) NOT NULL,
  `race` TINYINT(4) NOT NULL COMMENT 'Race Id',
  `expansion` TINYINT(4) NOT NULL COMMENT 'Expansion for race activation',
  PRIMARY KEY (`realmId`,`race`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

INSERT INTO realm_races VALUES ('1', '1', '0');
INSERT INTO realm_races VALUES ('1', '2', '0');
INSERT INTO realm_races VALUES ('1', '3', '0');
INSERT INTO realm_races VALUES ('1', '4', '0');
INSERT INTO realm_races VALUES ('1', '5', '0');
INSERT INTO realm_races VALUES ('1', '6', '0');
INSERT INTO realm_races VALUES ('1', '7', '0');
INSERT INTO realm_races VALUES ('1', '8', '0');
INSERT INTO realm_races VALUES ('1', '9', '3');
INSERT INTO realm_races VALUES ('1', '10', '1');
INSERT INTO realm_races VALUES ('1', '11', '1');
INSERT INTO realm_races VALUES ('1', '22', '3');
INSERT INTO realm_races VALUES ('1', '24', '4');
INSERT INTO realm_races VALUES ('1', '25', '4');
INSERT INTO realm_races VALUES ('1', '26', '4');