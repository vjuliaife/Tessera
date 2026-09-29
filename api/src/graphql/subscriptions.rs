use async_graphql::Result;
use futures::{Stream, StreamExt};
use redis::AsyncCommands;
use serde::de::DeserializeOwned;
use std::pin::Pin;

use super::{AssetTransfer, DividendEvent};

pub type RedisEventStream<T> = Pin<Box<dyn Stream<Item = T> + Send>>;

async fn redis_stream<T>(channel: &'static str) -> Result<RedisEventStream<T>>
where
    T: DeserializeOwned + Send + 'static,
{
    let client = redis::Client::open("redis://127.0.0.1/")?;
    let mut pubsub = client.get_async_pubsub().await?;
    pubsub.subscribe(channel).await?;

    let stream = pubsub
        .into_on_message()
        .filter_map(|message| async move {
            message
                .get_payload::<String>()
                .ok()
                .and_then(|payload| serde_json::from_str::<T>(&payload).ok())
        });

    Ok(Box::pin(stream))
}

pub async fn asset_transfers() -> Result<RedisEventStream<AssetTransfer>> {
    redis_stream("soroban:transfers").await
}

pub async fn dividend_events() -> Result<RedisEventStream<DividendEvent>> {
    redis_stream("soroban:dividends").await
}

pub async fn publish_asset_transfer(event: &AssetTransfer) -> Result<()> {
    let client = redis::Client::open("redis://127.0.0.1/")?;
    let mut connection = client.get_multiplexed_async_connection().await?;

    let payload = serde_json::to_string(event)?;
    connection
        .publish::<_, _, ()>("soroban:transfers", payload)
        .await?;

    Ok(())
}

pub async fn publish_dividend_event(event: &DividendEvent) -> Result<()> {
    let client = redis::Client::open("redis://127.0.0.1/")?;
    let mut connection = client.get_multiplexed_async_connection().await?;

    let payload = serde_json::to_string(event)?;
    connection
        .publish::<_, _, ()>("soroban:dividends", payload)
        .await?;

    Ok(())
}
