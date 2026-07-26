"""Base Agent class wrapping OpenAI SDK."""

from __future__ import annotations

import json
import os
from typing import Any, Optional

from openai import AsyncOpenAI


class Agent:
    """Wrapper around OpenAI chat completions with structured output support."""

    def __init__(
        self,
        name: str,
        system_prompt: str,
        model: str = "gpt-4o",
        temperature: float = 0.3,
        max_tokens: int = 4096,
        client: Optional[AsyncOpenAI] = None,
    ):
        self.name = name
        self.system_prompt = system_prompt
        self.model = model
        self.temperature = temperature
        self.max_tokens = max_tokens
        self._client = client

    @property
    def client(self) -> AsyncOpenAI:
        if self._client is None:
            kwargs = {
                "api_key": os.environ.get("OPENAI_API_KEY"),
                "timeout": float(os.environ.get("MESH_AGENT_API_TIMEOUT", "120")),
                "max_retries": 2,
            }
            base_url = os.environ.get("OPENAI_BASE_URL")
            if base_url:
                kwargs["base_url"] = base_url
            self._client = AsyncOpenAI(**kwargs)
        return self._client

    def _build_messages(self, user_message: str) -> list[dict[str, Any]]:
        return [
            {"role": "system", "content": self.system_prompt},
            {"role": "user", "content": user_message},
        ]

    async def run(self, user_message: str, **kwargs) -> str:
        """Send a message and return the text response."""
        messages = self._build_messages(user_message)
        response = await self.client.chat.completions.create(
            model=self.model,
            messages=messages,
            temperature=kwargs.get("temperature", self.temperature),
            max_tokens=kwargs.get("max_tokens", self.max_tokens),
        )
        return response.choices[0].message.content or ""

    async def run_structured(
        self, user_message: str, output_schema: dict[str, Any], **kwargs
    ) -> dict[str, Any]:
        """Send a message and return a parsed JSON response matching the schema."""
        schema_instruction = f"""
You MUST respond with a valid JSON object that conforms to this schema:
{json.dumps(output_schema, indent=2)}

Do NOT include any text outside the JSON object. Do NOT wrap in markdown code blocks.
Your entire response must be the raw JSON object.
"""
        full_message = f"{user_message}\n\n{schema_instruction}"
        messages = self._build_messages(full_message)

        response = await self.client.chat.completions.create(
            model=self.model,
            messages=messages,
            temperature=kwargs.get("temperature", self.temperature),
            max_tokens=kwargs.get("max_tokens", self.max_tokens),
        )
        raw = (response.choices[0].message.content or "{}").strip()

        # Try direct parse
        try:
            return json.loads(raw)
        except json.JSONDecodeError:
            pass

        # Strip markdown code fences
        cleaned = raw
        if cleaned.startswith("```"):
            lines = cleaned.split("\n")
            lines = [l for l in lines if not l.startswith("```")]
            cleaned = "\n".join(lines).strip()

        # Try again
        try:
            return json.loads(cleaned)
        except json.JSONDecodeError:
            pass

        # Try to extract JSON object from the text
        import re
        match = re.search(r'\{.*\}', cleaned, re.DOTALL)
        if match:
            try:
                return json.loads(match.group())
            except json.JSONDecodeError:
                pass

        # Last resort: return empty dict
        return {}


class StrongAgent(Agent):
    """Agent using a strong reasoning model (for Strategist, Optimist, Skeptic, Programmer, Reviewer)."""
    def __init__(self, name: str, system_prompt: str, client: Optional[AsyncOpenAI] = None):
        model = os.environ.get("MESH_AGENT_STRONG_MODEL", "deepseek-chat")
        super().__init__(
            name=name,
            system_prompt=system_prompt,
            model=model,
            temperature=0.3,
            max_tokens=4096,
            client=client,
        )


class LightAgent(Agent):
    """Agent using a fast/cheap model (for Gatekeeper, Memory Keeper, etc.)."""
    def __init__(self, name: str, system_prompt: str, client: Optional[AsyncOpenAI] = None):
        model = os.environ.get("MESH_AGENT_LIGHT_MODEL", "deepseek-chat")
        super().__init__(
            name=name,
            system_prompt=system_prompt,
            model=model,
            temperature=0.2,
            max_tokens=2048,
            client=client,
        )
