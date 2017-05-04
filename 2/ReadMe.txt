MegaDecryptor
--------------

In this exercise you are required to decrypt the secret passphrase that was encrypted with the following scheme.
The encrypted buffer ("CipherText.bin") is encrypted by an alternating stream whose behavior is defined by the key.
The given key ("Key.bin") is an array of EncryptionStepDescriptors, where an EncryptionStepDescriptor is
defined as follows:

struct EncryptionStepDescriptor
{
	UINT8 operationCode;
	UINT8 operationParameter;
	UINT32 lengthToOperateOn;
};

Each EncryptionStepDescriptor describes a single step for a specific operation on the stream.

a) operationCode - Here are the operationCodes:
1) Xor = 0
2) Add = 1
3) Subtract = 2

b) operationParameter - Each operation code receives an operation parameter (operationParameter) and uses it with the current
byte in the stream to compute a binary operation. 

c) lengthToOperateOn - The amount of bytes in the stream to apply this binary operation to.

For example:
Given the stream(/"CipherText.bin"): "ccccccc" and the key: { {Add, 1, 4}, {Subtract, 2, 3} }
we should end up with: "ddddaaa"

If you have reached the end of the stream, but there are more operations to do according to the key
you have to apply the key in reverse.

For example:
Given the stream(/"CipherText.bin"): "aaaaaaaaaaeeeeehhhhhhhhhgggghh" and the key: 
{
		{
			Add,
			5,
			10
		},
		{
			Add,
			1,
			5
		},
		{
			Subtract,
			2,
			9
		},
		{
			Subtract,
			1,
			8
		}
	}
we should end up with: "ffffffffffffffffffffffffffffff"