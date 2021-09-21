#pragma once
#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Core/Collections/Stacktor.h"


namespace Jimara {
	// Basic Push/Pop with data stored on stack
	TEST(StacktorTest, PrimitiveTypes_Stack_PushPop) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			Stacktor<int, 4> values;
			const Stacktor<int, 4>& constValues = values;
			ASSERT_EQ(values.Size(), 0);

			values.Push(1);
			ASSERT_EQ(values.Size(), 1);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Push(3);
			ASSERT_EQ(values.Size(), 2);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Push(2);
			ASSERT_EQ(values.Size(), 3);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			ASSERT_EQ(values[2], 2);
			ASSERT_EQ(constValues[2], 2);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Push(2);
			ASSERT_EQ(values.Size(), 4);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			ASSERT_EQ(values[2], 2);
			ASSERT_EQ(constValues[2], 2);
			ASSERT_EQ(values[3], 2);
			ASSERT_EQ(constValues[3], 2);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 3);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			ASSERT_EQ(values[2], 2);
			ASSERT_EQ(constValues[2], 2);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 2);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 1);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 0);
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Basic push/pop with data stored on heap
	TEST(StacktorTest, PrimitiveTypes_Heap_PushPop) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			Stacktor<int, 0> values;
			const Stacktor<int, 0>& constValues = values;
			ASSERT_EQ(values.Size(), 0);
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Push(1);
			ASSERT_EQ(values.Size(), 1);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Push(3);
			ASSERT_EQ(values.Size(), 2);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Push(2);
			ASSERT_EQ(values.Size(), 3);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			ASSERT_EQ(values[2], 2);
			ASSERT_EQ(constValues[2], 2);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 2);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 1);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 0);
			EXPECT_FALSE(constValues.StoredOnStack());
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Basic push/pop with data stored on heap or stack
	TEST(StacktorTest, PrimitiveTypes_PushPop) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			Stacktor<int, 1> values;
			const Stacktor<int, 1>& constValues = values;
			ASSERT_EQ(values.Size(), 0);
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Push(1);
			ASSERT_EQ(values.Size(), 1);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_TRUE(constValues.StoredOnStack());

			values.Push(3);
			ASSERT_EQ(values.Size(), 2);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			ASSERT_EQ(values[1], 3);
			ASSERT_EQ(constValues[1], 3);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 1);
			ASSERT_EQ(values[0], 1);
			ASSERT_EQ(constValues[0], 1);
			for (size_t i = 0; i < values.Size(); i++) {
				ASSERT_EQ(values.Data()[i], values[i]);
				ASSERT_EQ(constValues.Data()[i], constValues.Data()[i]);
			}
			EXPECT_FALSE(constValues.StoredOnStack());

			values.Pop();
			ASSERT_EQ(values.Size(), 0);
			EXPECT_FALSE(constValues.StoredOnStack());
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Tests RemoveAt() method
	TEST(StacktorTest, PrimitiveTypes_RemoveAt) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			Stacktor<int> values;
			const size_t initialCount = 8;
			for (int i = 0; i < initialCount; i++)
				values.Push(i);

			EXPECT_EQ(values.Size(), initialCount);
			for (size_t i = 0; i < values.Size(); i++)
				EXPECT_EQ(values[i], i);

			const size_t erasedIndex = 4;
			values.RemoveAt(erasedIndex);
			ASSERT_EQ(values.Size(), initialCount - 1);
			for (int i = 0; i < erasedIndex; i++)
				EXPECT_EQ(values[i], i);
			for (int i = erasedIndex; i < values.Size(); i++)
				EXPECT_EQ(values[i], i + 1);
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Tests SetData (explicit and implicit)
	TEST(StacktorTest, PrimitiveTypes_SetData) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			{
				Stacktor<int> values;
				values.Push(9);
				values.Push(12);
				values.SetData({ 0, 1, 2 });
				EXPECT_EQ(values.Size(), 3);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i);
			}
			{
				Stacktor<int> values({ 0, 1, 2, 3 });
				EXPECT_EQ(values.Size(), 4);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i);
			}
			{
				Stacktor<int> values;
				values = { 0, 1, 2, 3, 4 };
				EXPECT_EQ(values.Size(), 5);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i);
			}
			{
				Stacktor<int> values({ 0, 1, 2, 3 });
				EXPECT_EQ(values.Size(), 4);
				values.SetData(values.Data(), 4);
				EXPECT_EQ(values.Size(), 4);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i);

				values.SetData(values.Data(), 3);
				EXPECT_EQ(values.Size(), 3);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i);

				values.SetData(values.Data() + 1, 2);
				EXPECT_EQ(values.Size(), 2);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i + 1);
			}
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Tests Resize (explicit and implicit)
	TEST(StacktorTest, PrimitiveTypes_Resize) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			{
				Stacktor<int> values;
				values.Resize(32);
				EXPECT_EQ(values.Size(), 32);
			}
			{
				Stacktor<int> values({ 0, 1, 2, 3, 4, 5, 6, 7 });
				EXPECT_EQ(values.Size(), 8);
				values.Resize(4);
				EXPECT_EQ(values.Size(), 4);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], i);
			}
			{
				Stacktor<int> values(32, 774);
				EXPECT_EQ(values.Size(), 32);
				for (size_t i = 0; i < values.Size(); i++)
					EXPECT_EQ(values[i], 774);
			}
			{
				Stacktor<int> values({ 0, 1, 2, 3 });
				values.Resize(8, 16);
				EXPECT_EQ(values.Size(), 8);
				for (size_t i = 0; i < 4; i++)
					EXPECT_EQ(values[i], i);
				for (size_t i = 4; i < values.Size(); i++)
					EXPECT_EQ(values[i], 16);
			}
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Any kind of copy/move
	TEST(StacktorTest, PrimitiveTypes_Assign) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			{
				Stacktor<int, 4> a({ 0, 1, 2, 3 });
				Stacktor<int, 4> b(a);
				EXPECT_EQ(a.Size(), 4);
				EXPECT_TRUE(a.StoredOnStack());
				EXPECT_TRUE(b.StoredOnStack());
				ASSERT_EQ(a.Size(), b.Size());
				for (size_t i = 0; i < a.Size(); i++)
					EXPECT_EQ(a[i], b[i]);
			}
			{

				Stacktor<int> a({ 0, 1, 2, 3 });
				Stacktor<int> b({ 4, 5, 6, 7 });
				b = a;
				EXPECT_EQ(a.Size(), 4);
				EXPECT_FALSE(a.StoredOnStack());
				EXPECT_FALSE(b.StoredOnStack());
				ASSERT_EQ(a.Size(), b.Size());
				for (size_t i = 0; i < a.Size(); i++)
					EXPECT_EQ(a[i], b[i]);
			}
			{
				Stacktor<int> a({ 0, 1, 2, 3 });
				Stacktor<int> b(std::move(a));
				EXPECT_EQ(a.Size(), 0);
				EXPECT_EQ(b.Size(), 4);
				for (size_t i = 0; i < b.Size(); i++)
					EXPECT_EQ(b[i], i);
			}
			{
				Stacktor<int> a({ 0, 1, 2, 3 });
				Stacktor<int> b({ 4, 5, 6, 7 });
				b = std::move(a);
				EXPECT_EQ(a.Size(), 0);
				EXPECT_EQ(b.Size(), 4);
				for (size_t i = 0; i < b.Size(); i++)
					EXPECT_EQ(b[i], i);
			}
		}
		EXPECT_TRUE(snapshot.Compare());
	}

	// Case, when the stored types also have some heap allocations
	TEST(StacktorTest, ComplexTypes_Memory) {
		Jimara::Test::Memory::MemorySnapshot snapshot;
		{
			Stacktor<Stacktor<int>, 4> data({ Stacktor<int>({0, 1, 2}), Stacktor<int>({3, 4, 5}) });
			Stacktor<Stacktor<int>, 4> startA;
			startA = data;
			Stacktor<Stacktor<int>, 4> startB(data);
			data.Push(Stacktor<int>({ 7, 8, 9, 10 }));
			Stacktor<int> toPush({ 9, 11, 23 });
			data.Push(toPush);
			data.Push(toPush);
			data.Pop();
			data.RemoveAt(1);
			data.Push(toPush);
			data.Push(toPush);
			data.Push(toPush);
			data.Push(toPush);
			Stacktor<Stacktor<int>, 4> clone = data;
			Stacktor<Stacktor<int>, 4> clonesClone = clone;
			data = Stacktor<Stacktor<int>, 4>();
			EXPECT_EQ(data.Size(), 0);
			data = std::move(clonesClone);
			EXPECT_EQ(data.Size(), 7);
			clonesClone = std::move(clone);
			clone = data;
			data = data;
			data.SetData(data.Data(), data.Size());
			EXPECT_EQ(data.Size(), 7);
			data.SetData(data.Data(), 5);
			EXPECT_EQ(data.Size(), 5);
			data.SetData(data.Data() + 1, 3);
			EXPECT_EQ(data.Size(), 3);
			data.SetData(data.Data() + 1, 2);
			EXPECT_EQ(data.Size(), 2);
			data = clone;
			EXPECT_EQ(data.Size(), 7);
			data.RequestCapacity(2, true);
			EXPECT_EQ(data.Size(), 4);
		}
		EXPECT_TRUE(snapshot.Compare());
	}
}
