package com.jinwo.vecdb;

/**
 * Search result: a single vector match.
 */
public class SearchResult {
    private final long id;
    private final float distance;

    public SearchResult(long id, float distance) {
        this.id = id;
        this.distance = distance;
    }

    public long getId() { return id; }
    public float getDistance() { return distance; }

    @Override
    public String toString() {
        return String.format("SearchResult{id=%d, distance=%.6f}", id, distance);
    }
}
