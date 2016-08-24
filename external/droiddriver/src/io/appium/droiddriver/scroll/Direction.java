/*
 * Copyright (C) 2013 DroidDriver committers
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package io.appium.droiddriver.scroll;

import static io.appium.droiddriver.scroll.Direction.PhysicalDirection.DOWN;
import static io.appium.droiddriver.scroll.Direction.PhysicalDirection.LEFT;
import static io.appium.droiddriver.scroll.Direction.PhysicalDirection.RIGHT;
import static io.appium.droiddriver.scroll.Direction.PhysicalDirection.UP;

/**
 * A namespace to hold interfaces and constants for scroll directions.
 */
public class Direction {
  /** Logical directions */
  public enum LogicalDirection {
    FORWARD {
      @Override
      public LogicalDirection reverse() {
        return BACKWARD;
      }
    },
    BACKWARD {
      @Override
      public LogicalDirection reverse() {
        return FORWARD;
      }
    };
    public abstract LogicalDirection reverse();
  }

  /** Physical directions */
  public enum PhysicalDirection {
    UP {
      @Override
      public PhysicalDirection reverse() {
        return DOWN;
      }

      @Override
      public Axis axis() {
        return Axis.VERTICAL;
      }
    },
    DOWN {
      @Override
      public PhysicalDirection reverse() {
        return UP;
      }

      @Override
      public Axis axis() {
        return Axis.VERTICAL;
      }
    },
    LEFT {
      @Override
      public PhysicalDirection reverse() {
        return RIGHT;
      }

      @Override
      public Axis axis() {
        return Axis.HORIZONTAL;
      }
    },
    RIGHT {
      @Override
      public PhysicalDirection reverse() {
        return LEFT;
      }

      @Override
      public Axis axis() {
        return Axis.HORIZONTAL;
      }
    };
    public abstract PhysicalDirection reverse();

    public abstract Axis axis();
  }

  public enum Axis {
    HORIZONTAL {
      private final PhysicalDirection[] directions = {LEFT, RIGHT};

      @Override
      public PhysicalDirection[] getPhysicalDirections() {
        return directions;
      }
    },
    VERTICAL {
      private final PhysicalDirection[] directions = {UP, DOWN};

      @Override
      public PhysicalDirection[] getPhysicalDirections() {
        return directions;
      }
    };

    public abstract PhysicalDirection[] getPhysicalDirections();
  }

  /**
   * Converts between PhysicalDirection and LogicalDirection. It's possible to
   * override this for RTL (right-to-left) views, for example.
   */
  public static abstract class DirectionConverter {

    /** Follows standard convention: up-to-down, left-to-right */
    public static final DirectionConverter STANDARD_CONVERTER = new DirectionConverter() {
      @Override
      public PhysicalDirection horizontalForwardDirection() {
        return RIGHT;
      }

      @Override
      public PhysicalDirection verticalForwardDirection() {
        return DOWN;
      }
    };

    /** Follows RTL convention: up-to-down, right-to-left */
    public static final DirectionConverter RTL_CONVERTER = new DirectionConverter() {
      @Override
      public PhysicalDirection horizontalForwardDirection() {
        return LEFT;
      }

      @Override
      public PhysicalDirection verticalForwardDirection() {
        return DOWN;
      }
    };

    public abstract PhysicalDirection horizontalForwardDirection();

    public abstract PhysicalDirection verticalForwardDirection();

    public final PhysicalDirection horizontalBackwardDirection() {
      return horizontalForwardDirection().reverse();
    }

    public final PhysicalDirection verticalBackwardDirection() {
      return verticalForwardDirection().reverse();
    }

    /** Converts PhysicalDirection to LogicalDirection */
    public final LogicalDirection toLogicalDirection(PhysicalDirection physicalDirection) {
      LogicalDirection forward = LogicalDirection.FORWARD;
      if (toPhysicalDirection(physicalDirection.axis(), forward) == physicalDirection) {
        return forward;
      }
      return forward.reverse();
    }

    /** Converts LogicalDirection to PhysicalDirection */
    public final PhysicalDirection toPhysicalDirection(Axis axis, LogicalDirection logicalDirection) {
      switch (axis) {
        case HORIZONTAL:
          switch (logicalDirection) {
            case BACKWARD:
              return horizontalBackwardDirection();
            case FORWARD:
              return horizontalForwardDirection();
          }
          break;
        case VERTICAL:
          switch (logicalDirection) {
            case BACKWARD:
              return verticalBackwardDirection();
            case FORWARD:
              return verticalForwardDirection();
          }
      }
      return null;
    }
  }

  private Direction() {}
}
