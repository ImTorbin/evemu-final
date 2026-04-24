-- Passive effect 1626 fails on the client with KeyError 6466 when parsing dogma
-- (basedogmalocation.StartPassiveEffects -> expression tree lookup in cfg).
-- Incomplete static imports often omit rows still referenced by dgmEffects.preExpression / postExpression chains.
--
-- This inserts a minimal leaf expression (DEFBOOL / operand 23) so config.BulkData.dgmexpressions
-- contains the key. Replace this row with the exact tuple from a full Crucible-era SDE if bonuses differ.
--
-- +migrate Up

INSERT INTO dgmExpressions (
    expressionID,
    operandID,
    arg1,
    arg2,
    expressionValue,
    description,
    expressionName,
    expressionTypeID,
    expressionGroupID,
    expressionAttributeID
)
SELECT
    6466,
    23,
    0,
    0,
    '1',
    'EVEmu stub: missing expression from partial SDE (see migration header)',
    'stub_missing_6466',
    0,
    0,
    0
WHERE NOT EXISTS (SELECT 1 FROM dgmExpressions WHERE expressionID = 6466);

-- +migrate Down

DELETE FROM dgmExpressions WHERE expressionID = 6466 AND expressionName = 'stub_missing_6466';
