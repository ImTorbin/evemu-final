-- +migrate Up
CREATE INDEX idx_mktOrders_region_bid_type_price ON mktOrders (regionID, bid, typeID, price);

-- +migrate Down
DROP INDEX idx_mktOrders_region_bid_type_price ON mktOrders;
