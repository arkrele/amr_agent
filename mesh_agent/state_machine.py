"""State machine for mesh adaptation orchestrator."""

from __future__ import annotations

from enum import Enum
from typing import ClassVar


class State(str, Enum):
    IDLE = "idle"
    INIT_SOLVE = "init_solve"
    ANALYZE = "analyze"
    DEBATE = "debate"
    GATE = "gate"
    EXECUTE = "execute"
    VERIFY = "verify"
    DECIDE = "decide"
    SUMMARIZE = "summarize"
    CLOSED = "closed"


class StateMachine:
    TRANSITIONS: ClassVar[dict[State, list[State]]] = {
        State.IDLE: [State.INIT_SOLVE],
        State.INIT_SOLVE: [State.ANALYZE, State.SUMMARIZE],
        State.ANALYZE: [State.DEBATE, State.DECIDE],
        State.DEBATE: [State.GATE],
        State.GATE: [State.EXECUTE, State.DECIDE],
        State.EXECUTE: [State.VERIFY, State.DECIDE],
        State.VERIFY: [State.DECIDE],
        State.DECIDE: [State.ANALYZE, State.SUMMARIZE],
        State.SUMMARIZE: [State.CLOSED],
        State.CLOSED: [],
    }

    def __init__(self, initial: State = State.IDLE):
        self.current = initial
        self.history: list[State] = [initial]

    def can_transition(self, to: State) -> bool:
        return to in self.TRANSITIONS.get(self.current, [])

    def transition(self, to: State) -> None:
        if not self.can_transition(to):
            raise IllegalTransitionError(
                f"Cannot transition from {self.current.value} to {to.value}. "
                f"Allowed: {[s.value for s in self.TRANSITIONS.get(self.current, [])]}"
            )
        self.current = to
        self.history.append(to)

    @property
    def is_terminal(self) -> bool:
        return self.current == State.CLOSED


class IllegalTransitionError(Exception):
    """Raised when a state transition is not allowed."""
    pass
