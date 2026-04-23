-- Improve market asks aggregation query performance.
-- +migrate Up
CREATE INDEX idx_mktOrders_system_bid_type_price ON mktOrders (solarSystemID, bid, typeID, price);
CREATE INDEX idx_mktOrders_station_bid_type_price ON mktOrders (stationID, bid, typeID, price);

-- +migrate Down
DROP INDEX idx_mktOrders_system_bid_type_price ON mktOrders;
DROP INDEX idx_mktOrders_station_bid_type_price ON mktOrders;
