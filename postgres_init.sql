DROP INDEX IF EXISTS id_index;
DROP TABLE IF EXISTS test_table;
CREATE TABLE test_table (id_col INT, payload VARCHAR(252));
CREATE UNIQUE INDEX id_index ON test_table USING btree (id_col);
