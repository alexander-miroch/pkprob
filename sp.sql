DROP PROCEDURE IF EXISTS vpip;
DELIMITER $$
CREATE PROCEDURE vpip (
	in playerid BIGINT,
	out vpip decimal(6,2)
) READS SQL DATA COMMENT 'Determine vpip'
BEGIN
	DECLARE no_more_rows BOOLEAN;
	DECLARE loop_cntr INT DEFAULT 0;
	DECLARE num_rows INT DEFAULT 0;
	DECLARE action ENUM('BET','CHECK','RAISE','RERAISE','FOLD','CALL','RECALL');
	DECLARE gameid BIGINT;
	DECLARE vpip_cur CURSOR FOR
		SELECT h.action, h.gameid FROM hands h 
			WHERE h.playerid=playerid AND h.state="PREFLOP";
	DECLARE CONTINUE HANDLER FOR NOT FOUND
		SET no_more_rows = TRUE;

	OPEN vpip_cur;
	SELECT FOUND_ROWS() INTO num_rows;
		
	the_loop: LOOP
		FETCH vpip_cur INTO action, gameid;

		IF no_more_rows THEN
			CLOSE vpip_cur;
			LEAVE the_loop;
		END IF;

		select action, gameid, num_rows;		

		SET loop_cntr = loop_cntr + 1;
	END LOOP the_loop;

	select num_rows into @a;

END
$$

DELIMITER ;
