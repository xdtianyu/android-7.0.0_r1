# Adding the unique constraint will lead to the index being destroyed and a new
# unique index being created.
# To not rely on this implicit behavior, we explicitly delete the old index,
# and then create the new indexes.
# This is not really needed for the upwards migration, but if we can't be sure
# about the indexes names, it gets harder to do the DOWN migration later.
# Therefore we do this magic manually.
UP_SQL = """
ALTER TABLE afe_shards_labels DROP FOREIGN KEY shard_label_id_fk;
ALTER TABLE afe_shards_labels DROP INDEX shard_label_id_fk;
ALTER TABLE `afe_shards_labels` ADD UNIQUE `shard_label_id_uc` (`label_id`);
ALTER TABLE `afe_shards_labels` ADD CONSTRAINT shard_label_id_fk
        FOREIGN KEY (`label_id`) REFERENCES `afe_labels` (`id`);
"""

# Normally removing unique constraints is done just by deleting the index.
# This doesn't work here, as the index is also needed for the foreign key.
# Making an index back non-unique doesn't work in mysql.
# Therefore delete the foreign key, delete the index, re-add the foreign key.
DOWN_SQL = """
ALTER TABLE afe_shards_labels DROP FOREIGN KEY shard_label_id_fk;
ALTER TABLE afe_shards_labels DROP INDEX shard_label_id_uc;
ALTER TABLE `afe_shards_labels` ADD CONSTRAINT shard_label_id_fk
        FOREIGN KEY (`label_id`) REFERENCES `afe_labels` (`id`);
"""
