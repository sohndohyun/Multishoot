CREATE DATABASE IF NOT EXISTS `multishoot_test` CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS `multishoot`.`accounts` (
    `username` VARCHAR(16) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `password_salt` BINARY(16) NOT NULL,
    `password_hash` BINARY(32) NOT NULL,
    `best_score` INT UNSIGNED NOT NULL DEFAULT 0,
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`username`)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `multishoot_test`.`accounts` (
    `username` VARCHAR(16) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
    `password_salt` BINARY(16) NOT NULL,
    `password_hash` BINARY(32) NOT NULL,
    `best_score` INT UNSIGNED NOT NULL DEFAULT 0,
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`username`)
) ENGINE=InnoDB;

GRANT ALL PRIVILEGES ON `multishoot_test`.* TO 'multishoot'@'%';
FLUSH PRIVILEGES;
