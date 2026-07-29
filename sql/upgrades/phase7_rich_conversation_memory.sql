BEGIN;

ALTER TABLE messages
    ADD COLUMN IF NOT EXISTS provider_payload JSONB,
    ADD COLUMN IF NOT EXISTS tool_calls JSONB,
    ADD COLUMN IF NOT EXISTS tool_results JSONB,
    ADD COLUMN IF NOT EXISTS presentation JSONB,
    ADD COLUMN IF NOT EXISTS metadata JSONB,
    ADD COLUMN IF NOT EXISTS turn_id VARCHAR(64),
    ADD COLUMN IF NOT EXISTS turn_status VARCHAR(20);

DROP INDEX IF EXISTS idx_messages_conversation_created;
CREATE INDEX IF NOT EXISTS idx_messages_conversation_created
    ON messages(conversation_id, created_at, id);
CREATE INDEX IF NOT EXISTS idx_messages_conversation_turn
    ON messages(conversation_id, turn_id, id);

COMMIT;
