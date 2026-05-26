import Foundation

/// Search result: a single vector match.
public struct SearchResult: Equatable {
    public let id: UInt64
    public let distance: Float

    public init(id: UInt64, distance: Float) {
        self.id = id
        self.distance = distance
    }
}
